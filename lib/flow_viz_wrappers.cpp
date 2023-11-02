#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <memory>

#include <doca_flow.h>

#include <flow_viz.h>
#include <flow_viz_mermaid.h>

#define DOCA_FLOW_SO "libdoca_flow.so"

// Execute when library is loaded:
void __attribute__ ((constructor)) load_wrappers(void);
void __attribute__ ((destructor)) unload_wrappers(void);

// Destory when library is unloaded:
std::unique_ptr<FlowViz> flow_viz;
std::unique_ptr<FlowVizExporter> exporter;

void * log_dlopen(const char *file, int mode)
{
    void * handle = dlopen(file, mode);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    return handle;
}

void * log_dlsym(void *handle, const char *name)
{
    dlerror();
    void * res = dlsym(handle, name);
    char * error = dlerror();
    if (error) {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    return res;
}

// C++ does not allow implicit casts from void* to 
// whatever the type of func_ptr is, so capture its
// type as a template argument to provide the cast.
template<typename T>
void init_funcp(T &func_ptr, void *lib_handle, const char *sym_name)
{
    func_ptr = (T)log_dlsym(lib_handle, sym_name);
}


#define DECL_FUNCP(func) auto p_ ## func = &func
// example: auto p_doca_flow_init = &doca_flow_init

// Functions to wrap:
DECL_FUNCP(doca_flow_init);
DECL_FUNCP(doca_flow_destroy);
DECL_FUNCP(doca_flow_port_start);
DECL_FUNCP(doca_flow_port_stop);
DECL_FUNCP(doca_flow_pipe_create);
DECL_FUNCP(doca_flow_pipe_add_entry);
DECL_FUNCP(doca_flow_pipe_update_entry);
DECL_FUNCP(doca_flow_pipe_control_add_entry);
DECL_FUNCP(doca_flow_pipe_lpm_add_entry);
DECL_FUNCP(doca_flow_pipe_ordered_list_add_entry);
DECL_FUNCP(doca_flow_pipe_acl_add_entry);
DECL_FUNCP(doca_flow_pipe_hash_add_entry);
DECL_FUNCP(doca_flow_pipe_rm_entry);
DECL_FUNCP(doca_flow_pipe_destroy);
DECL_FUNCP(doca_flow_port_pipes_flush);
DECL_FUNCP(doca_flow_shared_resource_cfg);
DECL_FUNCP(doca_flow_shared_resources_bind);

void load_wrappers() {
    void * handle_doca_flow = log_dlopen("libdoca_flow.so", RTLD_LAZY);

#define INIT_FUNCP(lib_handle, func) init_funcp(p_ ## func, lib_handle, #func)

    INIT_FUNCP(handle_doca_flow, doca_flow_init);
    INIT_FUNCP(handle_doca_flow, doca_flow_destroy);
    INIT_FUNCP(handle_doca_flow, doca_flow_port_start);
    INIT_FUNCP(handle_doca_flow, doca_flow_port_stop);
    INIT_FUNCP(handle_doca_flow, doca_flow_port_pipes_flush);
    INIT_FUNCP(handle_doca_flow, doca_flow_pipe_create);
    INIT_FUNCP(handle_doca_flow, doca_flow_pipe_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_update_entry);
    INIT_FUNCP(handle_doca_flow, doca_flow_pipe_control_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_lpm_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_ordered_list_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_acl_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_hash_add_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_rm_entry);
	INIT_FUNCP(handle_doca_flow, doca_flow_pipe_destroy);
	INIT_FUNCP(handle_doca_flow, doca_flow_port_pipes_flush);
	INIT_FUNCP(handle_doca_flow, doca_flow_shared_resource_cfg);
	INIT_FUNCP(handle_doca_flow, doca_flow_shared_resources_bind);

	flow_viz = std::make_unique<FlowViz>();
	exporter = std::make_unique<MermaidExporter>();
}

void unload_wrappers() {
	flow_viz->export_flows(exporter.get());
}

doca_error_t
doca_flow_init(const struct doca_flow_cfg *cfg)
{
	doca_error_t res = (*p_doca_flow_init)(cfg);
	if (res == DOCA_SUCCESS) {
		flow_viz->flow_init();
	}
	return res;
}

void
doca_flow_destroy(void)
{
	(*p_doca_flow_destroy)();
	flow_viz->export_flows(exporter.get());
}

doca_error_t
doca_flow_port_start(const struct doca_flow_port_cfg *cfg,
		     struct doca_flow_port **port)
{
	doca_error_t res = (*p_doca_flow_port_start)(cfg, port);
	if (res == DOCA_SUCCESS) {
		flow_viz->port_started(cfg->port_id, *port);
	}
	return res;
}

doca_error_t
doca_flow_port_stop(struct doca_flow_port *port)
{
	doca_error_t res = (*p_doca_flow_port_stop)(port);
	if (res == DOCA_SUCCESS) {
		flow_viz->port_stopped(port);
	}
	return res;
}

void
doca_flow_port_pipes_flush(struct doca_flow_port *port)
{
	(*p_doca_flow_port_pipes_flush)(port);
	flow_viz->port_flushed(port);
}

doca_error_t
doca_flow_pipe_create(const struct doca_flow_pipe_cfg *cfg,
		const struct doca_flow_fwd *fwd,
		const struct doca_flow_fwd *fwd_miss,
		struct doca_flow_pipe **pipe)
{
    doca_error_t res = (*p_doca_flow_pipe_create)(cfg, fwd, fwd_miss, pipe);
	if (res == DOCA_SUCCESS) {
		flow_viz->pipe_created(cfg, fwd, fwd_miss, *pipe);
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
		flow_viz->entry_added(pipe, match, NULL, actions, fwd, monitor);
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
		flow_viz->entry_added(pipe, match, match_mask, actions, fwd, monitor);
	}
	return res;
}

doca_error_t
doca_flow_shared_resource_cfg(enum doca_flow_shared_resource_type type, uint32_t id,
			      struct doca_flow_shared_resource_cfg *cfg)
{
	doca_error_t res = (*p_doca_flow_shared_resource_cfg)(type, id, cfg);
	if (res == DOCA_SUCCESS) {
		flow_viz->resource_bound(type, id, cfg);
	}
	return res;
}
