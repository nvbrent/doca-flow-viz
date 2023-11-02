/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>

#define COUNTER_SPY_DECL_NO_EXTERN
#include <flow_viz_wrapper_decls.h>
#include <flow_viz_c.h>

#define DOCA_FLOW_SO "libdoca_flow.so"

void __attribute__ ((constructor)) init_logger(void);
void __attribute__ ((destructor)) close_logger(void);

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

void load_wrappers(void)
{
    void * handle_doca_flow = log_dlopen(DOCA_FLOW_SO, RTLD_LAZY);

#define INIT_PFUNC(func, lib_handle) p_ ## func = log_dlsym(lib_handle, #func)
    INIT_PFUNC(doca_flow_init,                   handle_doca_flow);
    INIT_PFUNC(doca_flow_destroy,                handle_doca_flow);
    INIT_PFUNC(doca_flow_port_start,             handle_doca_flow);
    INIT_PFUNC(doca_flow_port_stop,              handle_doca_flow);
    INIT_PFUNC(doca_flow_port_pipes_flush,       handle_doca_flow);
    INIT_PFUNC(doca_flow_pipe_create,            handle_doca_flow);
    INIT_PFUNC(doca_flow_pipe_add_entry,         handle_doca_flow);
    INIT_PFUNC(doca_flow_pipe_control_add_entry, handle_doca_flow);
    INIT_PFUNC(doca_flow_shared_resource_cfg,    handle_doca_flow);
}

void init_logger(void)
{
    load_wrappers();
}

void close_logger(void)
{
    flow_viz_export();
}
