#pragma once

#include <inttypes.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include <doca_flow.h>

using Fwd = struct doca_flow_fwd;
using Mon = struct doca_flow_monitor;
using Port = struct doca_flow_port;
using Pipe = struct doca_flow_pipe;
using Entry = struct doca_flow_entry;
using Match = struct doca_flow_match;
using PktActions = struct doca_flow_actions;
using CryptoCfg = struct doca_flow_resource_crypto_cfg;

struct Actions;
struct EntryActions;
struct PipeActions;
struct PortActions;
using PortActionMap = std::map<const Port *const, PortActions>;
using SharedCryptoFwd = std::map<uint32_t, CryptoCfg>;

struct Actions
{
    Fwd fwd = {};
    Fwd fwd_miss = {};
    Mon mon = {};
    Match match = {};
    Match match_mask = {};
    PktActions pkt_actions = {};
};
using ActionList = std::vector<Actions>;

struct EntryActions
{
    const Entry *entry_ptr = {};
    struct doca_flow_match match = {};
    Actions actions;
};

struct PipeActions
{
    const Pipe *pipe_ptr = {};
    const Port *port_ptr = {};
    struct doca_flow_pipe_attr attr = {};
    std::string attr_name;
    size_t instance_num;
    Actions pipe_actions;
    ActionList entries;
};

struct PortActions
{
    const Port *port_ptr = {};
    uint16_t port_id;
    std::map<std::string, size_t> pipe_name_count;
    std::map<const struct doca_flow_pipe*, PipeActions> pipe_actions;
};
