#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <doca_flow.h>
#include <flow_viz_c.h>
#include <flow_viz.h>
#include <flow_viz_util.h>

PortActionMap ports;
SharedCryptoFwd shared_crypto_map;

void flow_viz_port_started(
    uint16_t port_id,
    const struct doca_flow_port * port)
{
    auto &port_actions = ports[port];
    port_actions.port_id = port_id;
    port_actions.port_ptr = port;
}

void flow_viz_port_stopped(
    const struct doca_flow_port *port)
{
}

void flow_viz_port_flushed(
    const struct doca_flow_port *port)
{
}

void flow_viz_pipe_created(
    const struct doca_flow_pipe_cfg *cfg, 
    const struct doca_flow_fwd *fwd, 
    const struct doca_flow_fwd *fwd_miss, 
    const doca_flow_pipe *pipe)
{
    if (!cfg || !cfg->attr.name || !pipe)
        return;

    auto port_iter = ports.find(cfg->port);
    if (port_iter == ports.end())
        return;

    auto &port_actions = port_iter->second;
    auto &pipe_actions = port_actions.pipe_actions[pipe];
    pipe_actions.pipe_ptr = pipe;
    pipe_actions.port_ptr = cfg->port;
    pipe_actions.attr_name = cfg->attr.name;
    pipe_actions.attr = cfg->attr; // copy

    auto &counter = port_actions.pipe_name_count[pipe_actions.attr_name];
    pipe_actions.instance_num = counter++;

    if (cfg->monitor)
        pipe_actions.pipe_actions.mon = *cfg->monitor;
    if (fwd)
        pipe_actions.pipe_actions.fwd = *fwd;
    if (fwd_miss)
        pipe_actions.pipe_actions.fwd_miss = *fwd_miss;
    if (cfg->match)
        pipe_actions.pipe_actions.match = *cfg->match; // copy
    if (cfg->match_mask)
        pipe_actions.pipe_actions.match_mask = *cfg->match_mask; // copy
    // NOTE: only one action supported
    if (cfg->attr.nb_actions && cfg->actions && cfg->actions[0])
        pipe_actions.pipe_actions.pkt_actions = *cfg->actions[0];
}

void flow_viz_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_match *match,
    const struct doca_flow_match *match_mask,
    const struct doca_flow_actions *action,
    const struct doca_flow_fwd *fwd,
    const struct doca_flow_monitor *mon)
{
    for (auto &port_actions_iter : ports) {
        auto &port_actions = port_actions_iter.second;

        auto pipe_actions_iter = port_actions.pipe_actions.find(pipe);
        if (pipe_actions_iter == port_actions.pipe_actions.end()) {
            continue; // pipe not found
        }

        auto &pipe_actions = pipe_actions_iter->second;

        bool has_fwd = fwd && fwd->type;
        bool has_mon = mon && (
            mon->meter_type != DOCA_FLOW_RESOURCE_TYPE_NONE ||
            mon->counter_type != DOCA_FLOW_RESOURCE_TYPE_NONE ||
            mon->shared_mirror_id != 0 ||
            mon->aging_enabled
        );
        bool has_crypto = pipe_actions.pipe_actions.pkt_actions.crypto.action_type != DOCA_FLOW_CRYPTO_ACTION_NONE;
        if (!has_fwd && !has_mon && !has_crypto)
            return; // nothing to contribute

        auto &entry_actions = pipe_actions.entries.emplace_back();
        if (fwd)
            entry_actions.fwd = *fwd;
        if (mon)
            entry_actions.mon = *mon;
        if (match)
            entry_actions.match = *match;
        if (match_mask)
            entry_actions.match_mask = *match_mask;
        if (action)
            entry_actions.pkt_actions = *action;
        break;
    }
}

void flow_viz_resource_bound(
    enum doca_flow_shared_resource_type type, 
    uint32_t id,
    struct doca_flow_shared_resource_cfg *cfg)
{
    if (type == DOCA_FLOW_SHARED_RESOURCE_CRYPTO) {
        const auto &crypto_cfg = cfg->crypto_cfg;
        shared_crypto_map[id] = crypto_cfg;
    }
}

void flow_viz_init(void)
{

}

class MermaidExporter
{
public:
    virtual ~MermaidExporter() = default;

    std::ostream& export_ports(const PortActionMap& ports, std::ostream &out);

protected:
    using Braces = std::pair<std::string, std::string>;
    
    std::ostream& declare_port(
        const PortActions &port, 
        std::ostream &out);
    std::ostream& declare_pipe(
        const PortActions &port,
        const PipeActions &pipe, 
        std::ostream &out);
    std::ostream& declare_entry(
        const EntryActions &port, 
        std::ostream &out);
    std::ostream& declare_rss(
        const PortActions &port, 
        std::ostream &out);

    std::ostream& export_port(
        const PortActions &port, 
        std::ostream &out);
    std::ostream& export_pipe(
        const PipeActions &pipe, 
        std::ostream &out);
    std::ostream& export_pipe_fwd(
        const Fwd &fwd, 
        const std::string &arrow_str,
        const std::string &port_str,
        const std::string &pipe_str,
        const std::string &l3_l4_type,
        const std::string &action_str,
        bool is_secure,
        std::ostream &out);
    std::ostream& export_pipe_entry(
        const PipeActions &pipe,
        const EntryActions &entry,
        std::ostream &out);

    std::string stringify_port(const PortActions &port) const;
    std::string stringify_port(uint16_t port_id) const;
    std::string stringify_pipe(const PipeActions &pipe) const;
    std::string stringify_pipe_instance(const PipeActions &pipe, const Braces &braces) const;
    std::string stringify_rss(const Fwd &rss_action) const;

    // used to eliminate duplicate arrows
    using PortPipePair = std::pair<uint16_t, const Pipe*>;
    using PipePair = std::pair<const Pipe*, const Pipe*>;
    std::set<PortPipePair> port_pipe_pairs_exported;
    std::set<PipePair>     pipe_pairs_exported;
    std::set<const Pipe*>  drop_pipes_exported;
    
    bool is_exported(uint16_t port_id, const Pipe* pipe) const;
    bool is_exported(const Pipe* from_pipe, const Pipe* to_pipe) const;
    bool is_entry_redundant(const Pipe *pipe_ptr, const Fwd& actions) const;

    void set_exported(uint16_t port_id, const Pipe* pipe);
    void set_exported(const Pipe* from_pipe, const Pipe* to_pipe);
    void set_exported(const Pipe* pipe, const Fwd& actions);

    std::string tab = "    ";
    std::map<const Port*, std::string> port_decls;
    std::map<const Pipe*, std::string> pipe_decls;

    std::string drop_decl = "drop[[drop]]";

    std::string ingress = "ingress";
    std::string egress = "egress";
    std::string secure_ingress = "secure_ingress";
    std::string secure_egress = "secure_egress";

    std::string pipe_port_arrow = "-->";
    std::string fwd_arrow = "-->";
    std::string miss_arrow = "-.->";

    Braces port_braces = { "{{", "}}" };
    Braces pipe_braces = { "(", ")" };
    Braces quotes = { "\"", "\"" };
    Braces parens = { "(", ")" };
    Braces vbars = { "|", "|" };
    Braces empty_braces = { "", "" };

    std::string surround(std::string s, const Braces &braces) const {
        return braces.first + s + braces.second;
    }
};

std::string MermaidExporter::stringify_port(const PortActions &port) const
{
    return stringify_port(port.port_id);
}

std::string MermaidExporter::stringify_port(uint16_t port_id) const
{
    return "p" + std::to_string(port_id);
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

std::ostream& MermaidExporter::declare_port(const PortActions &port, std::ostream &out)
{
    if (port_decls.find(port.port_ptr) != port_decls.end())
        return out;

    const auto &port_str = port_decls[port.port_ptr] = stringify_port(port);

    std::vector<std::string> directions = { ingress, egress };
    if (!shared_crypto_map.empty()) {
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
    is_secure = false;
    if (actions.pkt_actions.crypto.proto_type == DOCA_FLOW_CRYPTO_PROTOCOL_NONE &&
        actions.pkt_actions.crypto.crypto_id == 0) {
        return actions.fwd;
    }

    is_secure = shared_crypto_map.find(
        actions.pkt_actions.crypto.crypto_id) != shared_crypto_map.end();

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
    
#if 0 // TODO: DOCA 2.5
    auto action_str = is_secure ?
        summarize_crypto(shared_crypto_map[pipe.pipe_actions.pkt_actions.security.crypto_id]) :
        summarize_actions(pipe.pipe_actions.pkt_actions);
#else
    auto action_str = summarize_actions(pipe.pipe_actions.pkt_actions);
#endif
    export_pipe_fwd(normal_fwd, fwd_arrow,  port_str, pipe_str, l3_l4_type, action_str, is_secure, out);
    export_pipe_fwd(miss_fwd,   miss_arrow, port_str, pipe_str, l3_l4_type, "",         is_secure, out);

    set_exported(pipe.pipe_ptr, normal_fwd);

    for (const auto &entry : pipe.entries) {
        l3_l4_type = summarize_l3_l4_types(pipe, &entry);
        const auto &entry_fwd = normal_or_crypto_fwd(entry, is_secure);
#if 0 // TODO: DOCA 2.5
        auto action_str = is_secure ?
            summarize_crypto(shared_crypto_map[entry.pkt_actions.security.crypto_id]) :
            summarize_actions(entry.pkt_actions);
#else
        auto action_str = summarize_actions(entry.pkt_actions);
#endif
        
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

std::ostream& MermaidExporter::export_ports(const PortActionMap& ports, std::ostream &out)
{
    out << "flowchart LR" << std::endl;
    for (const auto &port : ports) {
        declare_port(port.second, out);
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

void flow_viz_export(void)
{
    static bool export_done = false;
    if (!export_done) {
        MermaidExporter exporter;
        std::ofstream out("flows.md");
        out << "```mermaid" << std::endl;
        exporter.export_ports(ports, out);
        out << "```" << std::endl;
        export_done = true;
    }
}

