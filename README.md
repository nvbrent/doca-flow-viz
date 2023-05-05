# README

```
meson build
ninja -C build
build/template-doca-app -aXX:00.0,dv_flow_en=2 -aXX:00.1,dv_flow_en=2 -c0x3
(CTRL-C to exit)
```

Testing on the host:
```
export PKG_CONFIG_PATH=/opt/mellanox/doca/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/flexio/lib/pkgconfig:/opt/mellanox/dpdk/lib/x86_64-linux-gnu/pkgconfig:/opt/mellanox/grpc/lib/pkgconfig

ninja -C build && LD_PRELOAD=/root/repos/doca-counter-spy/build/lib/libdoca-counter-spy.so build/test/sample_flow_program -a17:00.0,dv_flow_en=2 -a17:00.1,dv_flow_en=2 -c0x1 -- -l 30
```