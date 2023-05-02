#pragma once

#include <doca_flow.h>

#ifdef __cplusplus
extern "C" {
#endif

void counter_spy_start_service(void);

void counter_spy_stop_service(void);

void counter_spy_port_started(
    uint16_t port_id,
    const struct doca_flow_port *port);

void counter_spy_port_stopped(
    const struct doca_flow_port *port);

void counter_spy_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const struct doca_flow_pipe *pipe);

void counter_spy_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_monitor *monitor, 
    const struct doca_flow_pipe_entry *entry);

#ifdef __cplusplus
}
#endif
