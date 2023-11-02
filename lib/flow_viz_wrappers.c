#include <doca_flow.h>
#include <flow_viz_wrapper_decls.h>
#include <flow_viz_c.h>

// Wrapper functions from doca_flow.h

doca_error_t
doca_flow_init(const struct doca_flow_cfg *cfg)
{
	doca_error_t res = (*p_doca_flow_init)(cfg);
	if (res == DOCA_SUCCESS) {
		flow_viz_init();
	}
	return res;
}

void
doca_flow_destroy(void)
{
	(*p_doca_flow_destroy)();
	flow_viz_export();
}

doca_error_t
doca_flow_port_start(const struct doca_flow_port_cfg *cfg,
		     struct doca_flow_port **port)
{
	doca_error_t res = (*p_doca_flow_port_start)(cfg, port);
	if (res == DOCA_SUCCESS) {
		flow_viz_port_started(cfg->port_id, *port);
	}
	return res;
}

doca_error_t
doca_flow_port_stop(struct doca_flow_port *port)
{
	doca_error_t res = (*p_doca_flow_port_stop)(port);
	if (res == DOCA_SUCCESS) {
		flow_viz_port_stopped(port);
	}
	return res;
}

void
doca_flow_port_pipes_flush(struct doca_flow_port *port)
{
	(*p_doca_flow_port_pipes_flush)(port);
	flow_viz_port_flushed(port);
}

doca_error_t
doca_flow_pipe_create(const struct doca_flow_pipe_cfg *cfg,
		const struct doca_flow_fwd *fwd,
		const struct doca_flow_fwd *fwd_miss,
		struct doca_flow_pipe **pipe)
{
    doca_error_t res = (*p_doca_flow_pipe_create)(cfg, fwd, fwd_miss, pipe);
	if (res == DOCA_SUCCESS) {
		flow_viz_pipe_created(cfg, fwd, fwd_miss, *pipe);
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
	if (res == DOCA_SUCCESS) {
		flow_viz_entry_added(pipe, match, NULL, actions, fwd, monitor);
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
			const struct doca_flow_actions *actions_mask,
			const struct doca_flow_action_descs *action_descs,
			const struct doca_flow_monitor *monitor,
			const struct doca_flow_fwd *fwd,
			void *usr_ctx,
			struct doca_flow_pipe_entry **entry)
{
    doca_error_t res = (*p_doca_flow_pipe_control_add_entry)(
        pipe_queue, priority, pipe, match, match_mask, 
		actions, actions_mask, action_descs, monitor, fwd, usr_ctx, entry);
	if (res == DOCA_SUCCESS) {
		flow_viz_entry_added(pipe, match, match_mask, actions, fwd, monitor);
	}
	return res;
}

doca_error_t
doca_flow_shared_resource_cfg(enum doca_flow_shared_resource_type type, uint32_t id,
			      struct doca_flow_shared_resource_cfg *cfg)
{
	doca_error_t res = (*p_doca_flow_shared_resource_cfg)(type, id, cfg);
	if (res == DOCA_SUCCESS) {
		flow_viz_resource_bound(type, id, cfg);
	}
	return res;
}
