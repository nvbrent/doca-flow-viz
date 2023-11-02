#pragma once

#include <inttypes.h>
#include <ostream>
#include <string>
#include <set>

#include <flow_viz_types.h>

class FlowVizExporter
{
public:
    FlowVizExporter() = default;
    virtual ~FlowVizExporter() = default;

    virtual std::ostream& start_file(std::ostream &out) = 0;

    virtual std::ostream& export_ports(
        const PortActionMap& ports, 
        const SharedCryptoFwd &shared_crypto_map,
        std::ostream &out) = 0;

    virtual std::ostream& end_file(std::ostream &out) = 0;
};
