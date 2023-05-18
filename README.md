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

Example of running the built-in sample:
```
LD_PRELOAD=build/lib/libdoca-flow-viz.so build/test/sample_flow_program -a17:00.0,dv_flow_en=2 -a17:00.1,dv_flow_en=2 -c0x1
```

Example output of the DOCA IPsec Security Gateway Sample Application (actual output!):
```mermaid
flowchart LR
    p0.ingress{{p0.ingress}}
    p0.egress{{p0.egress}}
    p1.ingress{{p1.ingress}}
    p1.egress{{p1.egress}}
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

p0.egress-->p0.ENCRYPT_PIPE-->    p0.secure_egress
    p0.secure_ingress-->p0.CONTROL_PIPE-->    p0.ingress
    p0.DECRYPT_SYNDROME_PIPE-->p1.egress
    p0.DECRYPT_SYNDROME_PIPE-.->drop
    p0.DECRYPT_PIPE0-.->drop
    p0.DECRYPT_PIPE0-->|IPV4|p0.DECRYPT_SYNDROME_PIPE
    p0.DECRYPT_PIPE1-.->drop
    p0.DECRYPT_PIPE1-->|IPV6|p0.DECRYPT_SYNDROME_PIPE
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