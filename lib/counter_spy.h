#pragma once

#include <inttypes.h>
#include <vector>

#include <doca_flow.h>

using EntryPtr = const struct doca_flow_pipe_entry*;
using PipePtr = const struct doca_flow_pipe*;

class EntryMon;
class PipeMon;
class PortMon;

struct EntryFlowStats
{
    bool valid = false;
    EntryPtr entry_ptr = nullptr;
    uint32_t shared_counter_id = 0;
    struct doca_flow_query total = {};
    struct doca_flow_query delta = {};
};
using FlowStatsList = std::vector<EntryFlowStats>;

struct PipeStats
{
    PipeMon *pipe_mon;
    FlowStatsList pipe_stats;
    FlowStatsList pipe_shared_counters;
};

struct PortStats
{
    PortMon *port_mon;
    std::map<std::string, PipeStats> port_stats;
    FlowStatsList port_shared_counters;
};

class EntryMon
{
public:
    explicit EntryMon();
    explicit EntryMon(
        const struct doca_flow_pipe_entry *entry_ptr, 
        const struct doca_flow_monitor *entry_mon);
    EntryFlowStats query_entry();

private:
    EntryPtr entry_ptr = nullptr;
    struct doca_flow_monitor mon = {};
    struct doca_flow_query stats = {};
};

class SharedCounterMon
{
public:
    SharedCounterMon();

    bool is_empty() const;

    void shared_counters_bound(
        uint32_t *res_array,
        uint32_t res_array_len);

    FlowStatsList query_entries();

private:
    std::map<uint32_t, EntryFlowStats> shared_counter_ids;
};

class PipeMon
{
public:
    explicit PipeMon();
    explicit PipeMon(
        const doca_flow_pipe *pipe,
        const doca_flow_pipe_attr &attr,
        const doca_flow_monitor *mon);

    PipeStats query_entries();

    std::string name() const;
    bool is_root() const;
    doca_flow_pipe_type type() const;

    void shared_counters_bound(
        uint32_t *res_array,
        uint32_t res_array_len);

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
    SharedCounterMon shared_counters;
};

class PortMon
{
public:
    explicit PortMon();
    explicit PortMon(
        uint16_t port_id,
        const struct doca_flow_port *port);
    ~PortMon();

    uint16_t port_id() const;

    PortStats query();

    void shared_counters_bound(
        uint32_t *res_array,
        uint32_t res_array_len);

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
    SharedCounterMon shared_counters;
    mutable std::mutex mutex;
};

// globals
extern std::map<const struct doca_flow_port *const, PortMon> ports;

