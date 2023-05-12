#include <string>
#include <map>
#include <mutex>
#include <random> // only used for faking stats
#include <thread>

#include <doca_flow.h>
#include <flow_viz_c.h>
#include <flow_viz.h>

std::map<const struct doca_flow_port *const, PortActions> ports;

void flow_viz_port_started(
    uint16_t port_id,
    const struct doca_flow_port * port)
{
    auto &port_actions = ports[port];
    port_actions.port_id = port_id;
    port_actions.port_ptr = port;
}

void flow_viz_port_stopped(
    const struct doca_flow_port *port)
{
}

void flow_viz_port_flushed(
    const struct doca_flow_port *port)
{
}

void flow_viz_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const struct doca_flow_fwd *fwd, 
    const struct doca_flow_fwd *fwd_miss, 
    const doca_flow_pipe *pipe)
{
    if (!cfg || !cfg->attr.name || !pipe)
        return;
    
    auto port_iter = ports.find(cfg->port);
    if (port_iter == ports.end())
        return;

    auto &port_actions = port_iter->second;
    auto &pipe_actions = port_actions.pipe_actions[pipe];
    pipe_actions.pipe_ptr = pipe;
    pipe_actions.attr_name = cfg->attr.name;
    pipe_actions.attr = cfg->attr; // copy
    if (cfg->monitor)
        pipe_actions.pipe_actions.mon = *cfg->monitor;
    if (fwd)
        pipe_actions.pipe_actions.fwd = *fwd;
    if (fwd_miss)
        pipe_actions.pipe_actions.fwd_miss = *fwd_miss;
}

void flow_viz_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_fwd *fwd,
    const struct doca_flow_monitor *mon, 
    const struct doca_flow_pipe_entry *entry)
{
    bool has_fwd = fwd && fwd->type;
    bool has_mon = mon && mon->flags;
    if (!has_fwd && !has_mon)
        return; // nothing to contribute

    for (auto &port_actions_iter : ports) {
        auto &port_actions = port_actions_iter.second;

        auto pipe_actions_iter = port_actions.pipe_actions.find(pipe);
        if (pipe_actions_iter == port_actions.pipe_actions.end()) {
            continue; // pipe not found
        }

        auto &pipe_actions = pipe_actions_iter->second;

        auto &entry_actions = pipe_actions.entries.emplace_back();
        if (fwd)
            entry_actions.fwd = *fwd;
        if (mon)
            entry_actions.mon = *mon;
        break;
    }
}

void flow_viz_start_service(void)
{
}

void flow_viz_stop_service(void)
{
}
