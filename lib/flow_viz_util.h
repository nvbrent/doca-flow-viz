#pragma once

#include <doca_flow.h>
#include <string>
#include <flow_viz_types.h>

enum doca_flow_l3_meta
accumulate_l3_type(enum doca_flow_l3_meta state, enum doca_flow_l3_meta input);

enum doca_flow_l4_meta
accumulate_l4_type(enum doca_flow_l4_meta state, enum doca_flow_l4_meta input);

enum doca_flow_l3_meta
summarize_l3_type(const Actions &actions, bool outer);

enum doca_flow_l4_meta
summarize_l4_type(const Actions &actions, bool outer);

enum doca_flow_l3_meta
summarize_l3_type(const PipeActions &pipe, const Actions *entry, bool outer);

enum doca_flow_l4_meta
summarize_l4_type(const PipeActions &pipe, const Actions *entry, bool outer);

std::string summarize_l3_l4_types(const PipeActions &pipe, const Actions *entry);

std::string summarize_actions(const PktActions &pkt_actions);

std::string summarize_crypto(const PktActions &pkt_actions);
