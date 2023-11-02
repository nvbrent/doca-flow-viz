#pragma once

#include <inttypes.h>
#include <ostream>
#include <string>
#include <set>

#include <flow_viz_types.h>
#include <flow_viz_exporter.h>

class MermaidExporter : public FlowVizExporter
{
public:
    MermaidExporter() = default;
    ~MermaidExporter() override = default;

    std::ostream& start_file(std::ostream &out) override;

    std::ostream& export_ports(
        const PortActionMap& ports, 
        const SharedCryptoFwd &shared_crypto_map,
        std::ostream &out) override;

    std::ostream& end_file(std::ostream &out) override;

protected:
    using Braces = std::pair<std::string, std::string>;
    
    std::ostream& declare_port(
        const PortActions &port, 
        bool include_secure_ingress_egress,
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
