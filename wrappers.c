#include <doca_flow.h>
#include <wrapper_decls.h>
#include <counter_spy.h>

// Wrapper functions from doca_flow.h

doca_error_t
doca_flow_port_start(const struct doca_flow_port_cfg *cfg,
		     struct doca_flow_port **port)
{
	doca_error_t res = (*p_doca_flow_port_start)(cfg, port);
	if (res == DOCA_SUCCESS) {
		counter_spy_port_started(cfg->port_id, *port);
	}
	return res;
}

doca_error_t
doca_flow_port_stop(struct doca_flow_port *port)
{
	doca_error_t res = (*p_doca_flow_port_stop)(port);
	if (res == DOCA_SUCCESS) {
		counter_spy_port_stopped(port);
	}
	return res;
}

bool is_counter_active(const struct doca_flow_monitor *mon)
{
	return (mon->flags & DOCA_FLOW_MONITOR_COUNT) || mon->shared_counter_id;
}

doca_error_t
doca_flow_pipe_create(const struct doca_flow_pipe_cfg *cfg,
		const struct doca_flow_fwd *fwd,
		const struct doca_flow_fwd *fwd_miss,
		struct doca_flow_pipe **pipe)
{
    doca_error_t res = (*p_doca_flow_pipe_create)(cfg, fwd, fwd_miss, pipe);
	if (res == DOCA_SUCCESS && cfg && cfg->monitor && is_counter_active(cfg->monitor)) {
		counter_spy_pipe_created(cfg, *pipe);
	}
    return res;
}

doca_error_t
doca_flow_pipe_add_entry(uint16_t pipe_queue,
			struct doca_flow_pipe *pipe,
			const struct doca_flow_match *match,
			const struct doca_flow_actions *actions,
			const struct doca_flow_monitor *monitor,
			const struct doca_flow_fwd *fwd,
			uint32_t flags,
			void *usr_ctx,
			struct doca_flow_pipe_entry **entry)
{
    doca_error_t res = (*p_doca_flow_pipe_add_entry)(
        pipe_queue, pipe, match, actions, monitor, fwd, 
        flags, usr_ctx, entry);
	if (res == DOCA_SUCCESS && monitor && is_counter_active(monitor)) {
		counter_spy_entry_added(pipe, monitor, *entry);
	}
	return res;
}

doca_error_t
doca_flow_pipe_control_add_entry(uint16_t pipe_queue,
			uint32_t priority,
			struct doca_flow_pipe *pipe,
			const struct doca_flow_match *match,
			const struct doca_flow_match *match_mask,
			const struct doca_flow_actions *actions,
			const struct doca_flow_action_descs *action_descs,
			const struct doca_flow_monitor *monitor,
			const struct doca_flow_fwd *fwd,
			struct doca_flow_pipe_entry **entry)
{
    doca_error_t res = (*p_doca_flow_pipe_control_add_entry)(
        pipe_queue, priority, pipe, match, match_mask, 
		actions, action_descs, monitor, fwd, entry);
	if (res == DOCA_SUCCESS && monitor && is_counter_active(monitor)) {
		counter_spy_entry_added(pipe, monitor, *entry);
	}
	return res;
}