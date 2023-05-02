#include <string>
#include <map>
#include <vector>

#include <doca_flow.h>
#include <counter_spy.h>

extern "C" void counter_spy_start_service(void);
extern "C" void counter_spy_stop_service(void);

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
        return stats;
    }
};

struct Pipe
{
    std::string name;
    PipePtr pipe;
    struct doca_flow_pipe_attr attr;
    std::map<EntryPtr, Entry> entries;

    FlowStatsList
    query_entries() const
    {
        FlowStatsList result;
        result.reserve(entries.size());

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

    PortStats
    query() const
    {
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

void counter_spy_start_service(void) {}
void counter_spy_stop_service(void) {}

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
    auto &pipe_counters = port_counters.pipes[pipe];
    pipe_counters.pipe = pipe;
    pipe_counters.attr = cfg->attr; // copy
    pipe_counters.name = cfg->attr.name; // copy
}

void counter_spy_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_monitor *monitor, 
    const struct doca_flow_pipe_entry *entry)
{
    for (auto port_counters_iter : ports) {
        auto &port_counters = port_counters_iter.second.pipes;
        auto pipe_counters_iter = port_counters.find(pipe);
        if (pipe_counters_iter == port_counters.end()) {
            continue;
        }
        auto &pipe_counters = pipe_counters_iter->second.entries;
        auto &entry_counters = pipe_counters[entry];
        entry_counters.entry_ptr = entry;
        entry_counters.mon = *monitor; // copy
    }
}
