#pragma once

#include <doca_flow.h>
#include <flow_viz_types.h>

class FlowVizExporter;

class FlowViz
{
public:
    FlowViz();
    ~FlowViz();

    // Exports all collected flow pipes
    void export_flows(FlowVizExporter *exporter);

    // doca_flow_init called
    void flow_init(void);

    // doca_flow_port_start called
    void port_started(
        uint16_t port_id,
        const struct doca_flow_port *port);

    // doca_flow_port_stop called
    void port_stopped(
        const struct doca_flow_port *port);

    // doca_flow_port_flush called
    void port_flushed(
        const struct doca_flow_port *port);

    // doca_flow_pipe_create called
    void pipe_created(
        const struct doca_flow_pipe_cfg *cfg, 
        const struct doca_flow_fwd *fwd, 
        const struct doca_flow_fwd *fwd_miss, 
        const struct doca_flow_pipe *pipe);

    // one of the family of doca_flow_xxx_pipe_entry_add called
    void entry_added(
        const struct doca_flow_pipe *pipe, 
        const struct doca_flow_match *match,
        const struct doca_flow_match *match_mask,
        const struct doca_flow_actions *action,
        const struct doca_flow_fwd *fwd,
        const struct doca_flow_monitor *monitor);

    // doca_flow_resource_bind called
    void resource_bound(
        enum doca_flow_shared_resource_type type, 
        uint32_t id,
        struct doca_flow_shared_resource_cfg *cfg);

private:
    bool export_done { false };

    PortActionMap ports;
};
