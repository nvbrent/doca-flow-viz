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

bool is_mac_set(const uint8_t *mac) {
    for (int i=0; i<DOCA_ETHER_ADDR_LEN; i++)
        if (mac[i])
            return true;
    return false;
}

bool is_src_addr_set(const doca_flow_header_format &hdr) {
    if (hdr.l3_type == DOCA_FLOW_L3_TYPE_IP4) {
        return hdr.ip4.src_ip != 0;
    }
    for (int32_t i=0; i<4; i++) {
        if (hdr.ip6.src_ip[i])
            return true;
    }
    return false;
}

bool is_dst_addr_set(const doca_flow_header_format &hdr) {
    if (hdr.l3_type == DOCA_FLOW_L3_TYPE_IP4) {
        return hdr.ip4.dst_ip != 0;
    }
    for (int32_t i=0; i<4; i++) {
        if (hdr.ip6.dst_ip[i])
            return true;
    }
    return false;
}

char action_sep = ',';

void add_action_str(std::string &actions, const std::string &token)
{
    if (actions.size())
        actions += action_sep;
    actions += token;
}

std::string summarize_actions(const PktActions &pkt_actions)
{
    // NOTE: Pipe- and Entry-level actions cannot be combined
    // NOTE: The following are not supported:
    // - meta.u32 and align
    // - inner header eth/ipv4/ipv6
    // - inner/outer L4 info (port num, tcp flags, etc.)
    std::string actions;
    if (pkt_actions.decap)
        add_action_str(actions, "DECAP");
    if (pkt_actions.pop)
        add_action_str(actions, "POP");
    if (pkt_actions.meta.pkt_meta || 
            pkt_actions.meta.port_meta ||
            pkt_actions.meta.mark || 
            pkt_actions.meta.ipsec_syndrome)
        add_action_str(actions, "+META");
    if (is_mac_set(pkt_actions.outer.eth.dst_mac))
        add_action_str(actions, "+DstMAC");
    if (is_mac_set(pkt_actions.outer.eth.src_mac))
        add_action_str(actions, "+SrcMAC");
    if (is_dst_addr_set(pkt_actions.outer))
        add_action_str(actions, "+DstIP");
    if (is_src_addr_set(pkt_actions.outer))
        add_action_str(actions, "+SrcIP");
    if (pkt_actions.has_encap)
        add_action_str(actions, "ENCAP");
    if (pkt_actions.has_push)
        add_action_str(actions, "PUSH");
    if (pkt_actions.security.proto_type != DOCA_FLOW_CRYPTO_PROTOCOL_NONE)
        add_action_str(actions, "CRYPTO");
    return actions;
}

std::string summarize_crypto(const CryptoCfg &pkt_actions)
{
    std::string actions;
    if (pkt_actions.proto_type == DOCA_FLOW_CRYPTO_PROTOCOL_NONE ||
        pkt_actions.action_type == DOCA_FLOW_CRYPTO_ACTION_NONE) {
        return actions;
    }
    // actions += pkt_actions.proto_type == DOCA_FLOW_CRYPTO_PROTOCOL_NISP ? "NISP/" : "ESP/";
    actions += pkt_actions.action_type == DOCA_FLOW_CRYPTO_ACTION_ENCRYPT ? "ENCR" : "DECR";
    if (pkt_actions.reformat_type == DOCA_FLOW_CRYPTO_REFORMAT_ENCAP)
        actions += "/ENCAP";
    else if (pkt_actions.reformat_type == DOCA_FLOW_CRYPTO_REFORMAT_DECAP)
        actions += "/DECAP";
    if (pkt_actions.net_type == DOCA_FLOW_CRYPTO_NET_TUNNEL)
        actions += "/TUNNEL";
    else if (pkt_actions.net_type == DOCA_FLOW_CRYPTO_NET_TRANSPORT)
        actions += "/XPORT";
    return actions;
}

