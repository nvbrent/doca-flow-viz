#include <flow_viz_mermaid.h>
#include <flow_viz_util.h>

std::ostream& MermaidExporter::start_file(std::ostream &out)
{
    return out << "```mermaid" << std::endl;
}

std::ostream& MermaidExporter::end_file(std::ostream &out)
{
    return out << "```" << std::endl;
}

std::string MermaidExporter::stringify_port(uint16_t port_id) const
{
    return "p" + std::to_string(port_id);
}

std::string MermaidExporter::stringify_port(const PortActions &port) const
{
    return stringify_port(port.port_id);
}

std::string MermaidExporter::stringify_pipe(const PipeActions& pipe) const
{
    return pipe.attr_name; // TODO: replace whitespace, etc.
}

std::string MermaidExporter::stringify_pipe_instance(
    const PipeActions &pipe,
    const Braces &braces) const
{
    return stringify_pipe(pipe) + surround(std::to_string(pipe.instance_num), braces);
}

std::string MermaidExporter::stringify_rss(const Fwd &rss_action) const
{
    return "rss"; // TODO: include queue indices, etc.
}

std::ostream& MermaidExporter::declare_port(
    const PortActions &port, 
    bool include_secure_ingress_egress,
    std::ostream &out)
{
    if (port_decls.find(port.port_ptr) != port_decls.end())
        return out;

    const auto &port_str = port_decls[port.port_ptr] = stringify_port(port);

    std::vector<std::string> directions = { ingress, egress };
    if (include_secure_ingress_egress) {
        directions.push_back(secure_ingress);
        directions.push_back(secure_egress);
    }

    for (const auto &direction : directions) {
        out << tab << "p" << port.port_id << "." << direction 
            << surround(port_str + "." + direction, port_braces)
            << std::endl;
    }

    return out;
}

std::ostream& MermaidExporter::declare_pipe(
    const PortActions &port,
    const PipeActions &pipe, 
    std::ostream &out)
{
    if (pipe_decls.find(pipe.pipe_ptr) != pipe_decls.end())
        return out; // this specific pipe has already been declared
    
    auto name_count_iter = port.pipe_name_count.find(pipe.attr_name);
    if (name_count_iter == port.pipe_name_count.end())
        return out;

    auto instance_count = name_count_iter->second;

    const auto &port_str = port_decls[pipe.port_ptr];

    auto &pipe_str = pipe_decls[pipe.pipe_ptr] = instance_count > 1 ? 
        stringify_pipe_instance(pipe, empty_braces) : 
        stringify_pipe(pipe);
    
    std::string pretty_pipe_name = instance_count > 1 ?
        stringify_pipe_instance(pipe, parens) : pipe_str;
    
    std::string full_pipe_name = port_str + "." + pipe_str;
    std::string full_pipe_pretty_name = port_str + "." + pretty_pipe_name;
        
    out << tab << full_pipe_name
        << surround(surround(full_pipe_pretty_name, quotes), pipe_braces)
        << std::endl;
    return out;
}

std::ostream& MermaidExporter::export_port(const PortActions &port, std::ostream &out)
{
    // Connect each port to its root pipe
    for (const auto &pipe : port.pipe_actions) {
        if (!pipe.second.attr.is_root)
            continue;

        const auto &port_str = port_decls[port.port_ptr];
        switch (pipe.second.attr.domain) {
        case DOCA_FLOW_PIPE_DOMAIN_DEFAULT:
            out << tab << port_str << "." << ingress
                << pipe_port_arrow 
                << port_str << "." << pipe_decls[pipe.second.pipe_ptr]
                << std::endl;
            break;
        case DOCA_FLOW_PIPE_DOMAIN_EGRESS:
            out << tab << port_str << "." << egress
                << pipe_port_arrow 
                << port_str << "." << pipe_decls[pipe.second.pipe_ptr]
                << std::endl;
            break;
        case DOCA_FLOW_PIPE_DOMAIN_SECURE_INGRESS:
            out << tab << port_str << "." << secure_ingress
                << pipe_port_arrow 
                << port_str << "." << pipe_decls[pipe.second.pipe_ptr]
                << std::endl;
            break;
        case DOCA_FLOW_PIPE_DOMAIN_SECURE_EGRESS:
            out << port_str << "." << egress
                << pipe_port_arrow 
                << port_str << "." << pipe_decls[pipe.second.pipe_ptr]
                << std::endl;
            // Note port-->secure_egress will be exported later, with action annotations
            break;
        default:
            break;
        }
    }

    for (const auto &pipe : port.pipe_actions) {
        export_pipe(pipe.second, out);
    }
    
    return out;
}

const Fwd& normal_or_crypto_fwd(const Actions &actions, bool &is_secure)
{
    is_secure = actions.pkt_actions.crypto.crypto_id != 0 ||
        actions.pkt_actions.crypto.proto_type != DOCA_FLOW_CRYPTO_PROTOCOL_NONE;

    return actions.fwd;
}

// Exports the fwd and fwd_miss actions of the pipe,
// as well as each of its pipe entries.
// Care is taken to ensure the same arrow never gets 
// drawn more than once.
std::ostream& MermaidExporter::export_pipe(const PipeActions &pipe, std::ostream &out)
{
    const auto &port_str = port_decls[pipe.port_ptr];
    std::string pipe_str = port_str + "." + pipe_decls[pipe.pipe_ptr];
    auto l3_l4_type = summarize_l3_l4_types(pipe, nullptr);

    bool is_secure = false;
    const auto &normal_fwd = normal_or_crypto_fwd(pipe.pipe_actions, is_secure);
    const auto &miss_fwd   = pipe.pipe_actions.fwd_miss;
    
    auto action_str = summarize_actions(pipe.pipe_actions.pkt_actions);
    export_pipe_fwd(normal_fwd, fwd_arrow,  port_str, pipe_str, l3_l4_type, action_str, is_secure, out);
    export_pipe_fwd(miss_fwd,   miss_arrow, port_str, pipe_str, l3_l4_type, "",         is_secure, out);

    set_exported(pipe.pipe_ptr, normal_fwd);

    for (const auto &entry : pipe.entries) {
        l3_l4_type = summarize_l3_l4_types(pipe, &entry);
        const auto &entry_fwd = normal_or_crypto_fwd(entry, is_secure);
        auto action_str = summarize_actions(entry.pkt_actions);
        
        if (!is_entry_redundant(pipe.pipe_ptr, entry_fwd)) {
            export_pipe_fwd(entry_fwd, fwd_arrow, port_str, pipe_str, l3_l4_type, action_str, is_secure, out);
            
            set_exported(pipe.pipe_ptr, entry_fwd);
        }
    }
    return out;
}

std::ostream& MermaidExporter::export_pipe_fwd(
    const Fwd &fwd, 
    const std::string &arrow_str,
    const std::string &port_str,
    const std::string &pipe_str,
    const std::string &l3_l4_type,
    const std::string &action_str,
    bool is_secure,
    std::ostream &out)
{
    std::string label = l3_l4_type;
    if (action_str.size()) {
        if (label.size())
            label += "/";
        label += action_str;
    }
    if (label.size()) {
        label = surround(label, vbars);
    }

    switch (fwd.type) {
    case DOCA_FLOW_FWD_NONE:
        break;
    case DOCA_FLOW_FWD_DROP:
        out << tab << pipe_str
            << arrow_str << "drop"
            << std::endl;
        break;
    case DOCA_FLOW_FWD_PORT:
        out << tab << pipe_str
            << arrow_str << label << stringify_port(fwd.port_id) << "." 
            // TODO: DOCA 2.5: Should we just use pipe.attr.domain here?
            << (is_secure ? secure_egress : egress)
            << std::endl;
        break;
    case DOCA_FLOW_FWD_PIPE:
        out << tab << pipe_str
            << arrow_str << label << port_str << "." << pipe_decls[fwd.next_pipe]
            << std::endl;
        break;
    case DOCA_FLOW_FWD_RSS:
    {
        auto rss_str = stringify_rss(fwd);
        out << tab << pipe_str 
            << arrow_str << rss_str
            << std::endl;
        break;
    }        
    default:
        break;
    }
    return out;
}

std::ostream& MermaidExporter::export_pipe_entry(
    const PipeActions &pipe,
    const EntryActions &entry,
    std::ostream &out)
{
    return out;
}

std::ostream& MermaidExporter::export_ports(
        const PortActionMap& ports, 
        const SharedCryptoFwd &shared_crypto_map,
        std::ostream &out)
{
    out << "flowchart LR" << std::endl;
    for (const auto &port : ports) {
        declare_port(port.second, !shared_crypto_map.empty(), out);
    }
    for (const auto &port : ports) {
        for (const auto &pipe : port.second.pipe_actions) {
            declare_pipe(port.second, pipe.second, out);
        }
    }
    out << tab << drop_decl << std::endl;

    out << std::endl;
    for (const auto &port : ports) {
        export_port(port.second, out);
    }
    return out;
}

bool MermaidExporter::is_exported(uint16_t port_id, const Pipe *pipe) const
{
    PortPipePair pair = { port_id, pipe };
    return port_pipe_pairs_exported.find(pair) != port_pipe_pairs_exported.end();
}

bool MermaidExporter::is_exported(const Pipe* from_pipe, const Pipe* to_pipe) const
{
    PipePair pair = { from_pipe, to_pipe };
    return pipe_pairs_exported.find(pair) != pipe_pairs_exported.end();
}

void MermaidExporter::set_exported(uint16_t port_id, const Pipe* pipe)
{
    PortPipePair pair = { port_id, pipe };
    port_pipe_pairs_exported.insert(pair);
}

void MermaidExporter::set_exported(const Pipe* from_pipe, const Pipe* to_pipe)
{
    PipePair pair = { from_pipe, to_pipe };
    pipe_pairs_exported.insert(pair);
}

void MermaidExporter::set_exported(const Pipe* pipe, const Fwd& fwd)
{
    switch (fwd.type) {
    case DOCA_FLOW_FWD_DROP:
        drop_pipes_exported.insert(pipe);
        break;
    case DOCA_FLOW_FWD_PIPE:
        pipe_pairs_exported.insert({pipe, fwd.next_pipe});
        break;
    case DOCA_FLOW_FWD_PORT:
        port_pipe_pairs_exported.insert({fwd.port_id, pipe});
    default:
        break;
    }
}

bool MermaidExporter::is_entry_redundant(const Pipe *pipe_ptr, const Fwd& fwd) const
{
    switch (fwd.type) {
    case DOCA_FLOW_FWD_DROP:
        return drop_pipes_exported.find(pipe_ptr) != drop_pipes_exported.end();
    case DOCA_FLOW_FWD_PIPE:
        return is_exported(pipe_ptr, fwd.next_pipe);
    case DOCA_FLOW_FWD_PORT:
        return is_exported(fwd.port_id, pipe_ptr);
    default:
        return false;
    }
}
