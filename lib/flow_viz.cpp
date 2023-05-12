#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include <doca_flow.h>
#include <flow_viz_c.h>
#include <flow_viz.h>

PortActionMap ports;

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
    if (cfg->monitor)
        pipe_actions.pipe_actions.mon = *cfg->monitor;
    if (fwd)
        pipe_actions.pipe_actions.fwd = *fwd;
    if (fwd_miss)
        pipe_actions.pipe_actions.fwd_miss = *fwd_miss;
}

void flow_viz_entry_added(
    const struct doca_flow_pipe *pipe, 
    const struct doca_flow_fwd *fwd,
    const struct doca_flow_monitor *mon, 
    const struct doca_flow_pipe_entry *entry)
{
    bool has_fwd = fwd && fwd->type;
    bool has_mon = mon && mon->flags;
    if (!has_fwd && !has_mon)
        return; // nothing to contribute

    for (auto &port_actions_iter : ports) {
        auto &port_actions = port_actions_iter.second;

        auto pipe_actions_iter = port_actions.pipe_actions.find(pipe);
        if (pipe_actions_iter == port_actions.pipe_actions.end()) {
            continue; // pipe not found
        }

        auto &pipe_actions = pipe_actions_iter->second;

        auto &entry_actions = pipe_actions.entries.emplace_back();
        if (fwd)
            entry_actions.fwd = *fwd;
        if (mon)
            entry_actions.mon = *mon;
        break;
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
    std::ostream& declare_port(const PortActions &port, std::ostream &out);
    std::ostream& declare_pipe(const PipeActions &port, std::ostream &out);
    std::ostream& declare_entry(const EntryActions &port, std::ostream &out);
    std::ostream& declare_rss(const PortActions &port, std::ostream &out);

    std::ostream& export_port(const PortActions &port, std::ostream &out);
    std::ostream& export_pipe(const PipeActions &pipe, std::ostream &out);
    std::ostream& export_pipe_dir(
        const PipeActions &pipe, 
        bool is_miss,
        std::ostream &out);
    std::ostream& export_pipe_entry(
        const PipeActions &pipe,
        const EntryActions &entry,
        std::ostream &out);

    std::string stringify_port(const PortActions &port) const;
    std::string stringify_port(uint16_t port_id) const;
    std::string stringify_pipe(const PipeActions &pipe) const;
    std::string stringify_rss(const Fwd &rss_action) const;

    std::string tab = "    ";
    std::map<const Port*, std::string> port_decls;
    std::map<const Pipe*, std::string> pipe_decls;

    std::string drop_decl = "drop[[drop]]";

    std::string ingress = "ingress";
    std::string egress = "egress";

    std::string pipe_port_arrow = "-->";
    std::string fwd_arrow = "-->";
    std::string miss_arrow = "-.->";

    using Braces = std::pair<std::string, std::string>;
    Braces port_braces = { "{{", "}}" };
    Braces pipe_braces = { "(", ")" };

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

std::string MermaidExporter::stringify_rss(const Fwd &rss_action) const
{
    return "rss"; // TODO: include queue indices, etc.
}

std::ostream& MermaidExporter::declare_port(const PortActions &port, std::ostream &out)
{
    if (port_decls.find(port.port_ptr) != port_decls.end())
        return out;

    const auto &port_str = port_decls[port.port_ptr] = stringify_port(port);

    for (const auto &direction : { ingress, egress }) {
        out << tab << "p" << port.port_id << "." << direction 
            << surround(port_str + "." + direction, port_braces)
            << std::endl;
    }

    return out;
}

std::ostream& MermaidExporter::declare_pipe(const PipeActions &pipe, std::ostream &out)
{
    if (pipe_decls.find(pipe.pipe_ptr) != pipe_decls.end())
        return out;

    const auto &pipe_str = pipe_decls[pipe.pipe_ptr] = stringify_pipe(pipe);
    const auto &port_str = port_decls[pipe.port_ptr];
    out << tab << port_str << "." << pipe.attr_name
        << surround(port_str + "." + pipe_str, pipe_braces)
        << std::endl;
    return out;
}

std::ostream& MermaidExporter::export_port(const PortActions &port, std::ostream &out)
{
    // Connect each port to its root pipe
    std::string direction = ingress;
    for (const auto &pipe : port.pipe_actions) {
        if (!pipe.second.attr.is_root)
            continue;
        
        const auto &port_str = port_decls[port.port_ptr];
        out << tab << port_str << "." << direction
            << pipe_port_arrow 
            << port_str << "." << pipe_decls[pipe.second.pipe_ptr]
            << std::endl;
    }

    for (const auto &pipe : port.pipe_actions) {
        export_pipe(pipe.second, out);
    }
    
    return out;
}

std::ostream& MermaidExporter::export_pipe(const PipeActions &pipe, std::ostream &out)
{
    for (bool b : { false, true })
        export_pipe_dir(pipe, b, out);
    return out;
}

std::ostream& MermaidExporter::export_pipe_dir(
    const PipeActions &pipe, 
    bool is_miss,
    std::ostream &out)
{
    const auto &fwd = is_miss ? pipe.pipe_actions.fwd_miss : pipe.pipe_actions.fwd;
    const auto &arrow = is_miss ? miss_arrow : fwd_arrow;

    const auto &port_str = port_decls[pipe.port_ptr];
    std::string pipe_str = port_str + "." + pipe_decls[pipe.pipe_ptr];

    switch (fwd.type) {
    case DOCA_FLOW_FWD_NONE:
        break;
    case DOCA_FLOW_FWD_DROP:
        out << tab << pipe_str
            << arrow << "drop"
            << std::endl;
        break;
    case DOCA_FLOW_FWD_PORT:
        out << tab << pipe_str
            << arrow << stringify_port(fwd.port_id) << "." << egress
            << std::endl;
        break;
    case DOCA_FLOW_FWD_PIPE:
        out << tab << pipe_str
            << arrow << port_str << "." << pipe_decls[fwd.next_pipe]
            << std::endl;
        break;
    case DOCA_FLOW_FWD_RSS:
    {
        auto rss_str = stringify_rss(fwd);
        out << tab << pipe_str 
            << arrow << rss_str
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
            declare_pipe(pipe.second, out);
        }
    }
    out << tab << drop_decl << std::endl;

    out << std::endl;
        for (const auto &port : ports) {
        export_port(port.second, out);
    }
    return out;
}

void flow_viz_export(void)
{
    MermaidExporter exporter;
    std::ofstream out("flows.md");
    out << "```mermaid" << std::endl;
    exporter.export_ports(ports, out);
    out << "```" << std::endl;
}

