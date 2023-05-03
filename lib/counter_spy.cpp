#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#include <doca_flow.h>
#include <counter_spy.h>

using EntryPtr = const struct doca_flow_pipe_entry*;
using PipePtr = const struct doca_flow_pipe*;

struct FlowStats
{
    bool valid;
    EntryPtr entry_ptr;
    uint32_t shared_counter_id;
    struct doca_flow_query query;
};

using FlowStatsList = std::vector<FlowStats>;

struct Entry
{
    EntryPtr entry_ptr;
    struct doca_flow_monitor mon;

    FlowStats
    query_entry() const
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
};

struct Pipe
{
    std::string name;
    PipePtr pipe;
    struct doca_flow_pipe_attr attr;
    struct doca_flow_monitor mon;
    std::map<EntryPtr, Entry> entries;

    FlowStatsList
    query_entries() const
    {
        FlowStatsList result;
        result.reserve(entries.size());

        printf("Pipe %s\n", name.c_str());
        for (const auto &entry : entries) {
            auto stats = entry.second.query_entry();
            if (stats.valid) {
                result.emplace_back(stats);
            }
        }

        return result;
    }

};

using PortStats = std::map<std::string, FlowStatsList>;

struct Port
{
    uint16_t port_id;
    const struct doca_flow_port *port;
    std::map<const PipePtr, Pipe> pipes;
    mutable std::mutex mutex;

    PortStats
    query() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        printf("Port %d\n", port_id);
        PortStats stats;
        for (const auto &pipe : pipes) {
            FlowStatsList entries = pipe.second.query_entries();
            if (!entries.empty()) {
                stats[pipe.second.name] = std::move(entries);
            }
        }
        return stats;
    }
};

std::map<const struct doca_flow_port *const, Port> ports;
std::vector<std::mutex> port_mutex;

static bool is_counter_active(const struct doca_flow_monitor *mon)
{
	return (mon->flags & DOCA_FLOW_MONITOR_COUNT) || mon->shared_counter_id;
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
    ports[port].port_id = port_id;
}

void counter_spy_port_stopped(
    const struct doca_flow_port *port)
{
    ports.erase(port);
}

void counter_spy_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const doca_flow_pipe *pipe)
{
    auto &port_counters = ports[cfg->port];
    std::lock_guard<std::mutex> lock(port_counters.mutex);
    auto &pipe_counters = port_counters.pipes[pipe];
    pipe_counters.pipe = pipe;
    pipe_counters.attr = cfg->attr; // copy
    if (cfg->monitor) {
        pipe_counters.mon = *cfg->monitor; // copy
    }
    pipe_counters.name = cfg->attr.name; // copy
}

void counter_spy_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_monitor *monitor, 
    const struct doca_flow_pipe_entry *entry)
{
    for (auto &port_counters_iter : ports) {
        auto &port_counters = port_counters_iter.second;
        std::lock_guard<std::mutex> lock(port_counters.mutex);

        auto pipe_counters_iter = port_counters.pipes.find(pipe);
        if (pipe_counters_iter == port_counters.pipes.end()) {
            continue; // pipe not found
        }

        auto &pipe_counters = pipe_counters_iter->second;

        bool counter_active_for_entry = monitor && is_counter_active(monitor);
        bool counter_active_for_pipe = is_counter_active(&pipe_counters_iter->second.mon);
        if (!counter_active_for_entry && !counter_active_for_pipe) {
            continue; // counter not active
        }

        auto &entry_counters = pipe_counters.entries[entry];
        entry_counters.entry_ptr = entry;
        if (monitor) {
            entry_counters.mon = *monitor; // copy
        }
        break;
    }
}
