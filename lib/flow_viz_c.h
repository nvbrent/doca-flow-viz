#pragma once

#include <doca_flow.h>

#ifdef __cplusplus
extern "C" {
#endif

void flow_viz_init(void);

void flow_viz_export(void);

void flow_viz_port_started(
    uint16_t port_id,
    const struct doca_flow_port *port);

void flow_viz_port_stopped(
    const struct doca_flow_port *port);

void flow_viz_port_flushed(
    const struct doca_flow_port *port);

void flow_viz_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const struct doca_flow_fwd *fwd, 
    const struct doca_flow_fwd *fwd_miss, 
    const struct doca_flow_pipe *pipe);

void flow_viz_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_match *match,
    const struct doca_flow_match *match_mask,
    const struct doca_flow_actions *action,
    const struct doca_flow_fwd *fwd,
    const struct doca_flow_monitor *monitor);

void flow_viz_resource_bound(
    enum doca_flow_shared_resource_type type, 
    uint32_t id,
    struct doca_flow_shared_resource_cfg *cfg);

#ifdef __cplusplus
}
#endif
