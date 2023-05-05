#pragma once

#include <inttypes.h>
#include <vector>

#include <doca_flow.h>

using EntryPtr = const struct doca_flow_pipe_entry*;
using PipePtr = const struct doca_flow_pipe*;

struct FlowStats
{
    bool valid = false;
    EntryPtr entry_ptr = nullptr;
    uint32_t shared_counter_id = 0;
    struct doca_flow_query query = {};
};

using FlowStatsList = std::vector<FlowStats>;

class EntryMon
{
public:
    explicit EntryMon();
    explicit EntryMon(
        const struct doca_flow_pipe_entry *entry_ptr, 
        const struct doca_flow_monitor *entry_mon);
    FlowStats query_entry() const;

private:
    EntryPtr entry_ptr = nullptr;
    struct doca_flow_monitor mon = {};
};

class PipeMon
{
public:
    explicit PipeMon();
    explicit PipeMon(
        const doca_flow_pipe *pipe,
        const doca_flow_pipe_attr &attr,
        const doca_flow_monitor *mon);

    FlowStatsList query_entries() const;

    std::string name() const;

    void entry_added(
        const struct doca_flow_pipe *pipe, 
        const struct doca_flow_monitor *monitor, 
        const struct doca_flow_pipe_entry *entry);

    static bool is_counter_active(
        const struct doca_flow_monitor *mon);

private:
    std::string attr_name;
    PipePtr pipe = nullptr;
    struct doca_flow_pipe_attr attr = {};
    struct doca_flow_monitor mon = {};
    std::map<EntryPtr, EntryMon> entries;
};

using PortStats = std::map<std::string, FlowStatsList>;

class PortMon
{
public:
    explicit PortMon();
    explicit PortMon(
        uint16_t port_id,
        const struct doca_flow_port *port);
    ~PortMon();

    uint16_t port_id() const;

    PortStats query() const;

    void pipe_created(
        const struct doca_flow_pipe_cfg *cfg, 
        const doca_flow_pipe *pipe);
    
    void port_flushed();
    
    PipeMon * find_pipe(const doca_flow_pipe *pipe);
    
    std::mutex& get_mutex() const;

private:
    uint16_t _port_id;
    const struct doca_flow_port *port;
    std::map<const PipePtr, PipeMon> pipes;
    mutable std::mutex mutex;
};

// globals
extern std::map<const struct doca_flow_port *const, PortMon> ports;

