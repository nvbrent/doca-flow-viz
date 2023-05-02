#pragma once

#include <doca_flow.h>

#ifndef COUNTER_SPY_DECL_NO_EXTERN // only defined in one source file!
#define COUNTER_SPY_EXTERN extern
#else
#define COUNTER_SPY_EXTERN
#endif

// Note this macro only works for certain C compilers; not for C++
#define DECL_PFUNC(func) COUNTER_SPY_EXTERN typeof(func) * p_ ## func

// Functions to wrap:
DECL_PFUNC(doca_flow_port_start);
DECL_PFUNC(doca_flow_port_stop);
DECL_PFUNC(doca_flow_pipe_create);
DECL_PFUNC(doca_flow_pipe_add_entry);
DECL_PFUNC(doca_flow_pipe_update_entry);
DECL_PFUNC(doca_flow_pipe_control_add_entry);
DECL_PFUNC(doca_flow_pipe_control_update_entry);
DECL_PFUNC(doca_flow_pipe_lpm_add_entry);
DECL_PFUNC(doca_flow_pipe_lpm_update_entry);
DECL_PFUNC(doca_flow_pipe_ordered_list_add_entry);
DECL_PFUNC(doca_flow_pipe_acl_add_entry);
DECL_PFUNC(doca_flow_pipe_hash_add_entry);
DECL_PFUNC(doca_flow_pipe_rm_entry);
DECL_PFUNC(doca_flow_pipe_destroy);
DECL_PFUNC(doca_flow_port_pipes_flush);
DECL_PFUNC(doca_flow_shared_resource_cfg);
DECL_PFUNC(doca_flow_shared_resources_bind);


