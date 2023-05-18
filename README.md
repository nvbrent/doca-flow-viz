# DOCA Flow Visualizer

The `flow-viz` library is a wrapper for `DOCA Flow`. It intercepts calls to create/destroy Pipes, and builds an internal representation of their forwarding actions to build a logical graph of their connections. This graph is then exported in Mermaid format.

Prerequisites: DOCA 2.0.2

## Building `flow-viz`:
```
meson build
ninja -C build
```

Viewing the results:

During development, it's handy to install a Mermaid [previewer plugin](https://marketplace.visualstudio.com/items?itemName=bierner.markdown-mermaid) for VSCode.

One common method of rendering Mermaid graphs is to include them in Markdown in a Mermaid code block (` ```mermaid ... ``` `); this displays correctly in a Web browser when the Markdown is rendered by [Gitlab](https://docs.gitlab.com/ee/user/markdown.html#mermaid).

Mermaid can also be embedded into HTML as described [here](https://mermaid.js.org/config/usage.html).

## Usage:

Simply prefix your regular DOCA application command line with `LD_PRELOAD=/path/to/libdocaflow-viz.so` and run your DOCA program normally.

## Examples

### Built-in Sample

Running the built-in sample:

(Use the PCI BDF appropriate to your system)
```
LD_PRELOAD=build/lib/libdoca-flow-viz.so build/test/sample_flow_program -a03:00.0,dv_flow_en=2 -a03:00.1,dv_flow_en=2 -c0x1
```

Output:
```mermaid
flowchart LR
    p0.ingress{{p0.ingress}}
    p0.egress{{p0.egress}}
    p1.ingress{{p1.ingress}}
    p1.egress{{p1.egress}}
    p0.HAIRPIN("p0.HAIRPIN")
    p0.SAMPLE_PIPE("p0.SAMPLE_PIPE")
    p0.NEXT_PIPE("p0.NEXT_PIPE")
    p0.ROOT_PIPE("p0.ROOT_PIPE")
    p1.HAIRPIN("p1.HAIRPIN")
    p1.SAMPLE_PIPE("p1.SAMPLE_PIPE")
    p1.NEXT_PIPE("p1.NEXT_PIPE")
    p1.ROOT_PIPE("p1.ROOT_PIPE")
    drop[[drop]]

    p0.ingress-->p0.ROOT_PIPE
    p0.HAIRPIN-->p1.egress
    p0.HAIRPIN-.->drop
    p0.SAMPLE_PIPE-->rss
    p0.SAMPLE_PIPE-.->p0.HAIRPIN
    p0.NEXT_PIPE-->|IPV4.TCP|p0.SAMPLE_PIPE
    p0.NEXT_PIPE-.->drop
    p0.ROOT_PIPE-->|IPV4|p0.NEXT_PIPE
    p0.ROOT_PIPE-.->drop
    p1.ingress-->p1.ROOT_PIPE
    p1.HAIRPIN-->p0.egress
    p1.HAIRPIN-.->drop
    p1.SAMPLE_PIPE-->rss
    p1.SAMPLE_PIPE-.->p1.HAIRPIN
    p1.NEXT_PIPE-->|IPV4.TCP|p1.SAMPLE_PIPE
    p1.NEXT_PIPE-.->drop
    p1.ROOT_PIPE-->|IPV4|p1.NEXT_PIPE
    p1.ROOT_PIPE-.->drop
```

### IPSec Security Gateway

Running the DOCA IPsec Security GW Sample Application:
```
LD_PRELOAD=build/lib/libdoca-flow-viz.so /opt/mellanox/doca/applications/security_gateway/bin/doca_security_gateway -s 03:00.0 -u 03:00.1 -m transport -c /opt/mellanox/doca/applications/security_gateway/bin/security_gateway_config.json -l 50
```

Actual output!

```mermaid
flowchart LR
    p0.ingress{{p0.ingress}}
    p0.egress{{p0.egress}}
    p0.secure_ingress{{p0.secure_ingress}}
    p0.secure_egress{{p0.secure_egress}}
    p1.ingress{{p1.ingress}}
    p1.egress{{p1.egress}}
    p1.secure_ingress{{p1.secure_ingress}}
    p1.secure_egress{{p1.secure_egress}}
    p0.ENCRYPT_PIPE("p0.ENCRYPT_PIPE")
    p0.DECRYPT_SYNDROME_PIPE("p0.DECRYPT_SYNDROME_PIPE")
    p0.DECRYPT_PIPE0("p0.DECRYPT_PIPE(0)")
    p0.DECRYPT_PIPE1("p0.DECRYPT_PIPE(1)")
    p0.CONTROL_PIPE("p0.CONTROL_PIPE")
    p1.HAIRPIN_PIPE0("p1.HAIRPIN_PIPE(0)")
    p1.HAIRPIN_PIPE1("p1.HAIRPIN_PIPE(1)")
    p1.HAIRPIN_PIPE2("p1.HAIRPIN_PIPE(2)")
    p1.HAIRPIN_PIPE3("p1.HAIRPIN_PIPE(3)")
    p1.SRC_IP6_PIPE0("p1.SRC_IP6_PIPE(0)")
    p1.SRC_IP6_PIPE1("p1.SRC_IP6_PIPE(1)")
    p1.CONTROL_PIPE("p1.CONTROL_PIPE")
    drop[[drop]]

p0.egress-->p0.ENCRYPT_PIPE
    p0.secure_ingress-->p0.CONTROL_PIPE
    p0.ENCRYPT_PIPE-->|ENCR/ENCAP/XPORT|p0.secure_egress
    p0.DECRYPT_SYNDROME_PIPE-->p1.egress
    p0.DECRYPT_SYNDROME_PIPE-.->drop
    p0.DECRYPT_PIPE0-.->drop
    p0.DECRYPT_PIPE0-->|IPV4/DECR/DECAP/XPORT|p0.DECRYPT_SYNDROME_PIPE
    p0.DECRYPT_PIPE1-.->drop
    p0.DECRYPT_PIPE1-->|IPV6/DECR/DECAP/XPORT|p0.DECRYPT_SYNDROME_PIPE
    p0.CONTROL_PIPE-->|IPV4|p0.DECRYPT_PIPE0
    p0.CONTROL_PIPE-->|IPV6|p0.DECRYPT_PIPE1
    p1.ingress-->p1.CONTROL_PIPE
    p1.HAIRPIN_PIPE0-->|IPV4.TCP/+META|p0.egress
    p1.HAIRPIN_PIPE1-->|IPV4.UDP/+META|p0.egress
    p1.HAIRPIN_PIPE2-->|IPV6.TCP/+META|p0.egress
    p1.HAIRPIN_PIPE3-->|IPV6.UDP/+META|p0.egress
    p1.SRC_IP6_PIPE0-->|IPV6.TCP|p1.HAIRPIN_PIPE2
    p1.SRC_IP6_PIPE1-->|IPV6.UDP|p1.HAIRPIN_PIPE3
    p1.CONTROL_PIPE-->|IPV4.TCP|p1.HAIRPIN_PIPE0
    p1.CONTROL_PIPE-->|IPV4.UDP|p1.HAIRPIN_PIPE1
    p1.CONTROL_PIPE-->|IPV6.TCP|p1.SRC_IP6_PIPE0
    p1.CONTROL_PIPE-->|IPV6.UDP|p1.SRC_IP6_PIPE1
```

### Future

Example output of a hypothetical application (work in progress):
```mermaid
flowchart LR
    p0.ingress{{p0.ingress}}
    p0.egress{{p0.egress}}
    p1.ingress{{p1.ingress}}
    p1.egress{{p1.egress}}
    p0.root(ROOT_PIPE)
    p0.acl_ip4(p0.ACL_IP4)
    p0.acl_ip6(p0.ACL_IP6)
    p0.hairpin(p0.HAIRPIN)
    p1.root(ROOT_PIPE)
    p1.acl_ip4(p1.ACL_IP4)
    p1.acl_ip6(p1.ACL_IP6)
    p1.hairpin(p1.HAIRPIN)
    rss_0_3[[RSS 0..3]]
    drop[[drop]]

    p0.ingress-->p0.root
    p0.root-->p0.acl_ip4
    p0.root-->p0.acl_ip6
    p0.root-->p0.hairpin
    p0.acl_ip4-->|count,meter|rss_0_3
    p0.acl_ip6-->|count,meter|rss_0_3
    p0.acl_ip4-.->drop
    p0.acl_ip6-.->drop
    p0.hairpin-->|count,meter|p1.egress

    p1.ingress-->p1.root
    p1.root-->p1.acl_ip4
    p1.root-->p1.acl_ip6
    p1.root-->p1.hairpin
    p1.acl_ip4-->|count,meter|rss_0_3
    p1.acl_ip6-->|count,meter|rss_0_3
    p1.acl_ip4-.->drop
    p1.acl_ip6-.->drop
    p1.hairpin-->|count,meter|p0.egress
```