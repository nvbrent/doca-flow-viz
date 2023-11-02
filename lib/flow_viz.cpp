#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <doca_flow.h>
#include <flow_viz_c.h>
#include <flow_viz.h>
#include <flow_viz_mermaid.h>

PortActionMap ports;
SharedCryptoFwd shared_crypto_map;

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
    pipe_actions.port_ptr = cfg->port;
    pipe_actions.attr_name = cfg->attr.name;
    pipe_actions.attr = cfg->attr; // copy

    auto &counter = port_actions.pipe_name_count[pipe_actions.attr_name];
    pipe_actions.instance_num = counter++;

    if (cfg->monitor)
        pipe_actions.pipe_actions.mon = *cfg->monitor;
    if (fwd)
        pipe_actions.pipe_actions.fwd = *fwd;
    if (fwd_miss)
        pipe_actions.pipe_actions.fwd_miss = *fwd_miss;
    if (cfg->match)
        pipe_actions.pipe_actions.match = *cfg->match; // copy
    if (cfg->match_mask)
        pipe_actions.pipe_actions.match_mask = *cfg->match_mask; // copy
    // NOTE: only one action supported
    if (cfg->attr.nb_actions && cfg->actions && cfg->actions[0])
        pipe_actions.pipe_actions.pkt_actions = *cfg->actions[0];
}

void flow_viz_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_match *match,
    const struct doca_flow_match *match_mask,
    const struct doca_flow_actions *action,
    const struct doca_flow_fwd *fwd,
    const struct doca_flow_monitor *mon)
{
    for (auto &port_actions_iter : ports) {
        auto &port_actions = port_actions_iter.second;

        auto pipe_actions_iter = port_actions.pipe_actions.find(pipe);
        if (pipe_actions_iter == port_actions.pipe_actions.end()) {
            continue; // pipe not found
        }

        auto &pipe_actions = pipe_actions_iter->second;

        bool has_fwd = fwd && fwd->type;
        bool has_mon = mon && (
            mon->meter_type != DOCA_FLOW_RESOURCE_TYPE_NONE ||
            mon->counter_type != DOCA_FLOW_RESOURCE_TYPE_NONE ||
            mon->shared_mirror_id != 0 ||
            mon->aging_enabled
        );
        bool has_crypto = pipe_actions.pipe_actions.pkt_actions.crypto.action_type != DOCA_FLOW_CRYPTO_ACTION_NONE;
        if (!has_fwd && !has_mon && !has_crypto)
            return; // nothing to contribute

        auto &entry_actions = pipe_actions.entries.emplace_back();
        if (fwd)
            entry_actions.fwd = *fwd;
        if (mon)
            entry_actions.mon = *mon;
        if (match)
            entry_actions.match = *match;
        if (match_mask)
            entry_actions.match_mask = *match_mask;
        if (action)
            entry_actions.pkt_actions = *action;
        break;
    }
}

void flow_viz_resource_bound(
    enum doca_flow_shared_resource_type type, 
    uint32_t id,
    struct doca_flow_shared_resource_cfg *cfg)
{
    if (type == DOCA_FLOW_SHARED_RESOURCE_CRYPTO) {
        const auto &crypto_cfg = cfg->crypto_cfg;
        shared_crypto_map[id] = crypto_cfg;
    }
}

void flow_viz_init(void)
{
    // nothing to do
}

void flow_viz_export(void)
{
    static bool export_done = false;
    if (!export_done) {
        MermaidExporter exporter;
        std::ofstream out("flows.md");
        out << "```mermaid" << std::endl;
        exporter.export_ports(ports, out);
        out << "```" << std::endl;
        export_done = true;
    }
}

