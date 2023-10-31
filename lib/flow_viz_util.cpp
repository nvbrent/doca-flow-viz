#include <flow_viz_util.h>

template<typename T_enum>
T_enum accumulate_l3_l4_type(T_enum state, T_enum input)
{
    return state == static_cast<T_enum>(0) ? input : state;
}

enum doca_flow_l3_meta
summarize_l3_type(const Actions &actions, bool outer)
{
    // If the pipe specifies a parser_meta L3 type, this indicates an explicit request
    // to match on this header value.
    auto l3_meta_type = DOCA_FLOW_L3_META_NONE;
    l3_meta_type = accumulate_l3_l4_type(l3_meta_type, outer ? actions.match_mask.parser_meta.outer_l3_type : actions.match_mask.parser_meta.inner_l3_type);
    l3_meta_type = accumulate_l3_l4_type(l3_meta_type, outer ? actions.match.parser_meta.outer_l3_type      : actions.match.parser_meta.inner_l3_type);
    if (l3_meta_type != DOCA_FLOW_L3_META_NONE)
        return l3_meta_type;

    // Otherwise, this pipe expects the parser_meta was already matched by a prior pipe,
    // and this pipe is specifying the L3 fields to match.
    auto l3_match_type = DOCA_FLOW_L3_TYPE_NONE;
    l3_match_type = accumulate_l3_l4_type(l3_match_type, outer ? actions.match_mask.outer.l3_type : actions.match_mask.inner.l3_type);
    l3_match_type = accumulate_l3_l4_type(l3_match_type, outer ? actions.match.outer.l3_type      : actions.match.inner.l3_type);
    switch (l3_match_type) {
    case DOCA_FLOW_L3_TYPE_IP4: return DOCA_FLOW_L3_META_IPV4;
    case DOCA_FLOW_L3_TYPE_IP6: return DOCA_FLOW_L3_META_IPV6;
    default: return DOCA_FLOW_L3_META_NONE;
    }
}

enum doca_flow_l4_meta
summarize_l4_type(const Actions &actions, bool outer)
{
    // If the pipe specifies a parser_meta L4 type, this indicates an explicit request
    // to match on this header value.
    auto l4_meta_type = DOCA_FLOW_L4_META_NONE;
    l4_meta_type = accumulate_l3_l4_type(l4_meta_type, outer ? actions.match_mask.parser_meta.outer_l4_type : actions.match_mask.parser_meta.inner_l4_type);
    l4_meta_type = accumulate_l3_l4_type(l4_meta_type, outer ? actions.match.parser_meta.outer_l4_type      : actions.match.parser_meta.inner_l4_type);
    if (l4_meta_type != DOCA_FLOW_L4_META_NONE)
        return l4_meta_type;

    // Otherwise, this pipe expects the parser_meta was already matched by a prior pipe,
    // and this pipe is specifying the L4 fields to match.
    auto l4_match_type = DOCA_FLOW_L4_TYPE_EXT_NONE;
    l4_match_type = accumulate_l3_l4_type(l4_match_type, outer ? actions.match_mask.outer.l4_type_ext : actions.match_mask.inner.l4_type_ext);
    l4_match_type = accumulate_l3_l4_type(l4_match_type, outer ? actions.match.outer.l4_type_ext      : actions.match.inner.l4_type_ext);
    switch (l4_match_type) {
    case DOCA_FLOW_L4_TYPE_EXT_TCP: return DOCA_FLOW_L4_META_TCP;
    case DOCA_FLOW_L4_TYPE_EXT_UDP: return DOCA_FLOW_L4_META_UDP;
    case DOCA_FLOW_L4_TYPE_EXT_ICMP: // fall through
    case DOCA_FLOW_L4_TYPE_EXT_ICMP6: return DOCA_FLOW_L4_META_ICMP;
    default: return DOCA_FLOW_L4_META_NONE;
    }
}

enum doca_flow_l3_meta
summarize_l3_type(const PipeActions &pipe, const Actions *entry, bool outer)
{
    auto l3_type = summarize_l3_type(pipe.pipe_actions, outer);

    if (entry) {
        l3_type = accumulate_l3_l4_type(l3_type, summarize_l3_type(*entry, outer));
    }
    return l3_type;
}

enum doca_flow_l4_meta
summarize_l4_type(const PipeActions &pipe, const Actions *entry, bool outer)
{
    auto l4_type = summarize_l4_type(pipe.pipe_actions, outer);
    if (entry) {
        l4_type = accumulate_l3_l4_type(l4_type, summarize_l4_type(*entry, outer));
    }
    return l4_type;
}

std::string summarize_l3_l4_types(const PipeActions &pipe, const Actions *entry)
{
    std::string type_str;

    auto l3_type_outer = summarize_l3_type(pipe, entry, true);
    if (l3_type_outer == DOCA_FLOW_L3_META_NONE)
        return type_str;
    type_str = l3_type_outer == DOCA_FLOW_L3_META_IPV4 ? "IPV4" : "IPV6";

    auto l4_type_outer = summarize_l4_type(pipe, entry, true);
    if (l4_type_outer != DOCA_FLOW_L4_META_NONE) {
        type_str += ".";
        type_str +=
            l4_type_outer == DOCA_FLOW_L4_META_TCP ? "TCP" :
            l4_type_outer == DOCA_FLOW_L4_META_UDP ? "UDP" :
            l4_type_outer == DOCA_FLOW_L4_META_ICMP ? "ICMP" :
            l4_type_outer == DOCA_FLOW_L4_META_ESP ? "ESP" : "UNKNOWN";
    }

    auto l3_type_inner = summarize_l3_type(pipe, entry, false);
    if (l3_type_inner == DOCA_FLOW_L3_META_NONE)
        return type_str;

    type_str += "/";
    type_str += l3_type_inner == DOCA_FLOW_L3_META_IPV4 ? "IPV4" : "IPV6";

    auto l4_type_inner = summarize_l4_type(pipe, entry, true);
    if (l4_type_inner != DOCA_FLOW_L4_META_NONE) {
        type_str += ".";
        type_str +=
            l4_type_inner == DOCA_FLOW_L4_META_TCP ? "TCP" :
            l4_type_inner == DOCA_FLOW_L4_META_UDP ? "UDP" :
            l4_type_inner == DOCA_FLOW_L4_META_ICMP ? "ICMP" :
            l4_type_inner == DOCA_FLOW_L4_META_ESP ? "ESP" : "UNKNOWN";
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
            // TODO: DOCA 2.5 pkt_actions.parser_meta.hash || 
            pkt_actions.parser_meta.port_meta ||
            pkt_actions.meta.mark || 
            pkt_actions.parser_meta.ipsec_syndrome)
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
#if 0 // TODO: DOCA 2.5
    if (pkt_actions.security.proto_type != DOCA_FLOW_CRYPTO_PROTOCOL_NONE)
        add_action_str(actions, "CRYPTO");
#endif
    return actions;
}

std::string summarize_crypto(const CryptoCfg &pkt_actions)
{
    std::string actions;
#if 0 // TODO: DOCA 2.5
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
#endif
    return actions;
}

