#include <flow_viz_util.h>


enum doca_flow_l3_type
accumulate_l3_type(enum doca_flow_l3_type state, enum doca_flow_l3_type input)
{
    return state == DOCA_FLOW_L3_TYPE_NONE ? input : state;
}

enum doca_flow_l4_type_ext
accumulate_l4_type(enum doca_flow_l4_type_ext state, enum doca_flow_l4_type_ext input)
{
    return state == DOCA_FLOW_L4_TYPE_EXT_NONE ? input : state;
}

enum doca_flow_l3_type
summarize_l3_type(const Actions &actions, bool outer)
{
    auto l3_type = DOCA_FLOW_L3_TYPE_NONE;
    l3_type = accumulate_l3_type(l3_type, outer ? actions.match_mask.outer.l3_type : actions.match_mask.inner.l3_type);
    l3_type = accumulate_l3_type(l3_type, outer ? actions.match.outer.l3_type      : actions.match.inner.l3_type);
    return l3_type;
}

enum doca_flow_l4_type_ext
summarize_l4_type(const Actions &actions, bool outer)
{
    auto l4_type = DOCA_FLOW_L4_TYPE_EXT_NONE;
    l4_type = accumulate_l4_type(l4_type, outer ? actions.match_mask.outer.l4_type_ext : actions.match_mask.inner.l4_type_ext);
    l4_type = accumulate_l4_type(l4_type, outer ? actions.match.outer.l4_type_ext      : actions.match.inner.l4_type_ext);
    return l4_type;
}

enum doca_flow_l3_type
summarize_l3_type(const PipeActions &pipe, const Actions *entry, bool outer)
{
    enum doca_flow_l3_type l3_type = summarize_l3_type(pipe.pipe_actions, outer);
    if (entry) {
        l3_type = accumulate_l3_type(l3_type, summarize_l3_type(*entry, outer));
    }
    return l3_type;
}

enum doca_flow_l4_type_ext
summarize_l4_type(const PipeActions &pipe, const Actions *entry, bool outer)
{
    auto l4_type = summarize_l4_type(pipe.pipe_actions, outer);
    if (entry) {
        l4_type = accumulate_l4_type(l4_type, summarize_l4_type(*entry, outer));
    }
    return l4_type;
}

std::string summarize_l3_l4_types(const PipeActions &pipe, const Actions *entry)
{
    std::string type_str;

    auto l3_type_outer = summarize_l3_type(pipe, entry, true);
    if (l3_type_outer == DOCA_FLOW_L3_TYPE_NONE)
        return type_str;
    type_str = l3_type_outer == DOCA_FLOW_L3_TYPE_IP4 ? "IPV4" : "IPV6";

    auto l4_type_outer = summarize_l4_type(pipe, entry, true);
    if (l4_type_outer != DOCA_FLOW_L4_TYPE_EXT_NONE) {
        type_str += ".";
        type_str +=
            l4_type_outer == DOCA_FLOW_L4_TYPE_EXT_TCP ? "TCP" :
            l4_type_outer == DOCA_FLOW_L4_TYPE_EXT_UDP ? "UDP" :
            l4_type_outer == DOCA_FLOW_L4_TYPE_EXT_ICMP ? "ICMP" :
            l4_type_outer == DOCA_FLOW_L4_TYPE_EXT_ICMP6 ? "ICMP6" : "UNKNOWN";
    }

    auto l3_type_inner = summarize_l3_type(pipe, entry, false);
    if (l3_type_inner == DOCA_FLOW_L3_TYPE_NONE)
        return type_str;

    type_str += "/";
    type_str += l3_type_inner == DOCA_FLOW_L3_TYPE_IP4 ? "IPV4" : "IPV6";

    auto l4_type_inner = summarize_l4_type(pipe, entry, true);
    if (l4_type_inner != DOCA_FLOW_L4_TYPE_EXT_NONE) {
        type_str += ".";
        type_str +=
            l4_type_inner == DOCA_FLOW_L4_TYPE_EXT_TCP ? "TCP" :
            l4_type_inner == DOCA_FLOW_L4_TYPE_EXT_UDP ? "UDP" :
            l4_type_inner == DOCA_FLOW_L4_TYPE_EXT_ICMP ? "ICMP" :
            l4_type_inner == DOCA_FLOW_L4_TYPE_EXT_ICMP6 ? "ICMP6" : "UNKNOWN";
    }

    return type_str;
}
