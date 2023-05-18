#pragma once

#include <doca_flow.h>
#include <string>
#include <flow_viz.h>

enum doca_flow_l3_type
accumulate_l3_type(enum doca_flow_l3_type state, enum doca_flow_l3_type input);

enum doca_flow_l4_type_ext
accumulate_l4_type(enum doca_flow_l4_type_ext state, enum doca_flow_l4_type_ext input);

enum doca_flow_l3_type
summarize_l3_type(const Actions &actions, bool outer);

enum doca_flow_l4_type_ext
summarize_l4_type(const Actions &actions, bool outer);

enum doca_flow_l3_type
summarize_l3_type(const PipeActions &pipe, const Actions *entry, bool outer);

enum doca_flow_l4_type_ext
summarize_l4_type(const PipeActions &pipe, const Actions *entry, bool outer);

std::string summarize_l3_l4_types(const PipeActions &pipe, const Actions *entry);

std::string summarize_actions(const PktActions &pkt_actions);

std::string summarize_crypto(const CryptoCfg &pkt_actions);
