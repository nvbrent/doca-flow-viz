#pragma once

#include <inttypes.h>
#include <vector>
#include <string>

#include <doca_flow.h>

using Fwd = struct doca_flow_fwd;
using Mon = struct doca_flow_monitor;

struct Actions
{
    Fwd fwd = {};
    Fwd fwd_miss = {};
    Mon mon = {};
};
using ActionList = std::vector<Actions>;

struct EntryActions
{
    const struct doca_flow_entry *entry_ptr = {};
    Actions actions;
};

struct PipeActions
{
    const struct doca_flow_pipe *pipe_ptr = {};
    struct doca_flow_pipe_attr attr = {};
    std::string attr_name;
    Actions pipe_actions;
    ActionList entries;
};

struct PortActions
{
    const struct doca_flow_port *port_ptr = {};
    uint16_t port_id;
    std::map<const struct doca_flow_pipe*, PipeActions> pipe_actions;
};

// globals
extern std::map<const struct doca_flow_port *const, PortActions> ports;

