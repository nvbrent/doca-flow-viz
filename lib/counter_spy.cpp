#include <string>
#include <map>
#include <mutex>
#include <random> // only used for faking stats
#include <thread>

#include <doca_flow.h>
#include <counter_spy_c.h>
#include <counter_spy.h>

#include <counter_spy.pb.h>
#include <counter_spy.grpc.pb.h>

const std::string COUNTER_SPY_FAKE_STATS_ENV_VAR = "COUNTER_SPY_FAKE_STATS";

bool fake_stats_enabled = false;
std::mt19937 rng; // don't bother with non-default seed
std::normal_distribution<> pkts_random_dist(0, 10000);
std::normal_distribution<> bytes_random_dist(0, 1000000);

std::map<const struct doca_flow_port *const, PortMon> ports;

struct doca_flow_query fake_stats()
{
    struct doca_flow_query stats;
    stats.total_bytes = (uint64_t)std::abs(bytes_random_dist(rng));
    stats.total_pkts  = (uint64_t)std::abs(pkts_random_dist(rng));
    return stats;
}

struct doca_flow_query operator+(
    const struct doca_flow_query &a, 
    const struct doca_flow_query &b)
{
    struct doca_flow_query c;
    c.total_bytes = a.total_bytes + b.total_bytes;
    c.total_pkts = a.total_pkts + b.total_pkts;
    return c;
}

struct doca_flow_query operator-(
    const struct doca_flow_query &a, 
    const struct doca_flow_query &b)
{
    struct doca_flow_query c;
    c.total_bytes = a.total_bytes - b.total_bytes;
    c.total_pkts = a.total_pkts - b.total_pkts;
    return c;
}

EntryMon::EntryMon() = default;

EntryMon::EntryMon(
    const struct doca_flow_pipe_entry *entry_ptr, 
    const struct doca_flow_monitor *entry_mon) : entry_ptr(entry_ptr)
{ 
    if (entry_mon) {
        this->mon = *entry_mon; // copy
    }
}

EntryFlowStats
EntryMon::query_entry()
{
    EntryFlowStats result = {};
    result.entry_ptr = entry_ptr;
    result.shared_counter_id = 0;

    if (fake_stats_enabled) {
        // Generate a fake delta and accumulate the total
        result.delta = fake_stats();
        this->stats = this->stats + result.delta;
        result.total = this->stats;
        result.valid = true;
    } else {
        // Query the total and compute the delta
        auto prev_stats = this->stats;
        auto res = doca_flow_query_entry(
            const_cast<struct doca_flow_pipe_entry *>(entry_ptr), 
            &result.total);
        result.valid = res == DOCA_SUCCESS;
        if (result.valid) {
            result.delta = result.total - prev_stats;
            this->stats = result.total;
        }
    }
    return result;
}

SharedCounterMon::SharedCounterMon() = default;

bool SharedCounterMon::is_empty() const
{
    return shared_counter_ids.empty();
}

void SharedCounterMon::shared_counters_bound(
    uint32_t *res_array,
    uint32_t res_array_len)
{
    for (uint32_t i=0; i<res_array_len; i++) {
        shared_counter_ids[res_array[i]].shared_counter_id = res_array[i];
    }
}

FlowStatsList SharedCounterMon::query_entries()
{
    FlowStatsList result;

    size_t n_shared = shared_counter_ids.size();

    if (n_shared == 0) {
        return result;
    }

    // Keep a copy of the counters before the query so
    // the delta can be computed
    auto prev_counters = shared_counter_ids; // copy

    // Prepare the vector of counter IDs for the batch query operation
    std::vector<uint32_t> counter_ids;
    counter_ids.reserve(n_shared);
    for (const auto &shared_ctr : shared_counter_ids) {
        counter_ids.push_back(shared_ctr.first);
    }

    // Prepare a vector for the output of the batch query:
    std::vector<doca_flow_shared_resource_result> query_results_array(n_shared);

    if (!fake_stats_enabled) {
        auto res = doca_flow_shared_resources_query(
            DOCA_FLOW_SHARED_RESOURCE_COUNT, counter_ids.data(),
            query_results_array.data(), n_shared);

        if (res != DOCA_SUCCESS) {
            return result;
        }
    }

    result.reserve(result.size() + n_shared);

    for (size_t i=0; i<n_shared; i++) {
        auto &shared_ctr = shared_counter_ids[counter_ids[i]];
        const auto &prev_ctr = prev_counters[counter_ids[i]];
        auto &result_stat = result.emplace_back();
        result_stat.shared_counter_id = counter_ids[i];

        if (fake_stats_enabled) {
            // Generate a fake delta and accumulate the total
            result_stat.delta = fake_stats();
            shared_ctr.total = shared_ctr.total + result_stat.delta;
            result_stat.total = shared_ctr.total;
        } else {
            // Update our internal state
            shared_ctr.total = query_results_array[i].counter;
            shared_ctr.delta = shared_ctr.total - prev_ctr.total;

            // Update the output message
            result_stat.total = shared_ctr.total;
            result_stat.delta = shared_ctr.delta;
        }
    }

    return result;
}

PipeMon::PipeMon() = default;

PipeMon::PipeMon(
    const doca_flow_pipe *pipe,
    const doca_flow_pipe_attr &attr,
    const doca_flow_monitor *pipe_mon) : 
        attr_name(attr.name), 
        pipe(pipe), 
        attr(attr)
{
    if (pipe_mon) {
        this->mon = *pipe_mon; // copy
    }
}

std::string PipeMon::name() const
{
    return attr_name;
}

bool PipeMon::is_root() const
{
    return attr.is_root;
}

doca_flow_pipe_type PipeMon::type() const
{
    return attr.type;
}

bool PipeMon::is_counter_active(const struct doca_flow_monitor *mon)
{
	return mon && (
        (mon->flags & DOCA_FLOW_MONITOR_COUNT) || 
        mon->shared_counter_id
    );
}

PipeStats
PipeMon::query_entries()
{
    PipeStats result;
    result.pipe_mon = this;
    result.pipe_stats.reserve(entries.size());

    //printf("Query: Pipe %s\n", attr_name.c_str());
    for (auto &entry : entries) {
        auto stats = entry.second.query_entry();
        if (stats.valid) {
            result.pipe_stats.emplace_back(stats);
        }
    }

    result.pipe_shared_counters = std::move(shared_counters.query_entries());

    return result;
}

void PipeMon::shared_counters_bound(
    uint32_t *res_array,
    uint32_t res_array_len)
{
    shared_counters.shared_counters_bound(res_array, res_array_len);
}

void PipeMon::entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_monitor *entry_monitor, 
    const struct doca_flow_pipe_entry *entry)
{
    if (is_counter_active(entry_monitor) ||
        is_counter_active(&this->mon)) 
    {
        entries[entry] = EntryMon(entry, entry_monitor);
    }
}

PortMon::PortMon() = default;

PortMon::PortMon(
    uint16_t port_id,
    const struct doca_flow_port *port) :
        _port_id(port_id),
        port(port)
{
}

PortMon::~PortMon()
{
    port_flushed();
}

void PortMon::port_flushed()
{
    std::lock_guard<std::mutex> lock(mutex);
    pipes.clear();
}

uint16_t PortMon::port_id() const
{
    return _port_id;
}

PortStats
PortMon::query()
{
    std::lock_guard<std::mutex> lock(mutex);
    //printf("Query: Port %d\n", _port_id);
    PortStats stats;
    stats.port_mon = this;
    for (auto &pipe : pipes) {
        auto entries = pipe.second.query_entries();
        if (!entries.pipe_stats.empty()) {
            stats.port_stats[pipe.second.name()] = std::move(entries);
        }
    }

    stats.port_shared_counters = std::move(shared_counters.query_entries());

    return stats;
}

void PortMon::shared_counters_bound(
    uint32_t *res_array,
    uint32_t res_array_len)
{
    shared_counters.shared_counters_bound(res_array, res_array_len);
}

void PortMon::pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const doca_flow_pipe *pipe)
{
    std::lock_guard<std::mutex> lock(mutex);
    pipes[pipe] = PipeMon(pipe, cfg->attr, cfg->monitor);
}

std::mutex& PortMon::get_mutex() const
{
    return mutex;
}

PipeMon *PortMon::find_pipe(const doca_flow_pipe *pipe)
{
    auto pipe_counters_iter = pipes.find(pipe);
    return (pipe_counters_iter == pipes.end()) ? nullptr : &pipe_counters_iter->second;
}

void counter_spy_port_started(
    uint16_t port_id,
    const struct doca_flow_port * port)
{
    ports.emplace(std::piecewise_construct,
        std::forward_as_tuple(port),
        std::forward_as_tuple(port_id, port));
}

void counter_spy_port_stopped(
    const struct doca_flow_port *port)
{
    ports.erase(port);
}

void counter_spy_port_flushed(
    const struct doca_flow_port *port)
{
    ports[port].port_flushed();
}

void counter_spy_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const doca_flow_pipe *pipe)
{
    auto &port_counters = ports[cfg->port];
    port_counters.pipe_created(cfg, pipe);
}

void counter_spy_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_monitor *monitor, 
    const struct doca_flow_pipe_entry *entry)
{
    for (auto &port_counters_iter : ports) {
        auto &port_counters = port_counters_iter.second;
        std::lock_guard<std::mutex> lock(port_counters.get_mutex());

        auto pipe_counters = port_counters.find_pipe(pipe);
        if (!pipe_counters) {
            continue; // pipe not found
        }

        pipe_counters->entry_added(pipe, monitor, entry);
        break;
    }
}

void counter_spy_shared_counters_bound(
    enum doca_flow_shared_resource_type type, 
    uint32_t *res_array,
    uint32_t res_array_len, 
    void *bindable_obj)
{
    // Determine whether bindable_obj is a port or pipe
    auto find_port = ports.find((doca_flow_port*)bindable_obj);
    if (find_port != ports.end()) {
        // found a port!
        find_port->second.shared_counters_bound(res_array, res_array_len);
        return;
    }

    for (auto &port_iter : ports) {
        auto &port_counters = port_iter.second;

        std::lock_guard<std::mutex> lock(port_counters.get_mutex());

        auto pipe_counters = port_counters.find_pipe((PipePtr)bindable_obj);
        if (!pipe_counters) {
            continue; // pipe not found
        }

        pipe_counters->shared_counters_bound(res_array, res_array_len);
        break;
    }
}
