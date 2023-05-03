#include <string>
#include <map>
#include <mutex>
#include <thread>

#include <doca_flow.h>
#include <counter_spy_c.h>
#include <counter_spy.h>

#include <counter_spy.pb.h>
#include <counter_spy.grpc.pb.h>

std::map<const struct doca_flow_port *const, PortMon> ports;

EntryMon::EntryMon() = default;

EntryMon::EntryMon(
    const struct doca_flow_pipe_entry *entry_ptr, 
    const struct doca_flow_monitor *entry_mon) : entry_ptr(entry_ptr)
{ 
    if (entry_mon) {
        this->mon = *entry_mon; // copy
    }
}

FlowStats
EntryMon::query_entry() const
{
    FlowStats stats;
    stats.entry_ptr = entry_ptr;
    stats.shared_counter_id = 0;
    auto res = doca_flow_query_entry(
        const_cast<struct doca_flow_pipe_entry *>(entry_ptr), 
        &stats.query);
    stats.valid = res == DOCA_SUCCESS;
    printf("Entry %p: valid: %d, pkts: %ld\n", entry_ptr, stats.valid, stats.query.total_pkts);
    return stats;
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

bool PipeMon::is_counter_active(const struct doca_flow_monitor *mon)
{
	return mon && (
        (mon->flags & DOCA_FLOW_MONITOR_COUNT) || 
        mon->shared_counter_id
    );
}

FlowStatsList
PipeMon::query_entries() const
{
    FlowStatsList result;
    result.reserve(entries.size());

    printf("Pipe %s\n", attr_name.c_str());
    for (const auto &entry : entries) {
        auto stats = entry.second.query_entry();
        if (stats.valid) {
            result.emplace_back(stats);
        }
    }

    return result;
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
        port_id(port_id),
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

PortStats
PortMon::query() const
{
    std::lock_guard<std::mutex> lock(mutex);
    printf("Port %d\n", port_id);
    PortStats stats;
    for (const auto &pipe : pipes) {
        FlowStatsList entries = pipe.second.query_entries();
        if (!entries.empty()) {
            stats[pipe.second.name()] = std::move(entries);
        }
    }
    return stats;
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

void counter_spy_svc_loop()
{
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        for (const auto &port: ports) {
            port.second.query();
        }
    }
}

void counter_spy_start_service(void)
{
    std::thread(counter_spy_svc_loop).detach();
}

void counter_spy_stop_service(void)
{

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
