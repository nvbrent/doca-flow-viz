# DOCA Flow Counter-Spy

The `counter-spy` library is a wrapper for `DOCA Flow`. It intercepts calls to create/destroy Pipes, and keeps an internal database of counters which can be queried. It also listens on a gRPC connection and allows a client to retrieve the state of all the counters.

(`TODO`: shared counters; pipe miss-counters)

Prerequisites: DOCA 2.0.2

Building `counter-spy`:
```
meson build
ninja -C build
```

Preparing the Python UI environment:
```
pip3 install rich grpcio grpcio-tools
cd ui
./generate_grpc.sh
cd ..
```

Usage:

Simply prefix your regular DOCA application command line with `LD_PRELOAD=/path/to/libcounter-spy.so` and run your DOCA program normally.

Example of running the built-in sample:
```
LD_PRELOAD=build/lib/libdoca-counter-spy.so build/test/sample_flow_program -a17:00.0,dv_flow_en=2 -a17:00.1,dv_flow_en=2 -c0x1
```

In another terminal (local or remote), run the UI to see updates:
```
python3 counter_spy_gui.py [-r host-running-doca]
```

The `-r` remote-host flag is optional; the default is localhost.

Note that the port number, 50051, is not presently configurable.

Example output, with counters: delta-packets, delta-bytes, total-packets, and total-bytes:
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Port 0                                                     ┃ Port 1                                                     ┃
┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┩
│ Pipes                                                      │ Pipes                                                      │
│ ├── NEXT_PIPE (PIPE_BASIC)                                 │ ├── NEXT_PIPE (PIPE_BASIC)                                 │
│ │   └── ┏━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │ │   └── ┏━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │
│ │       ┃ Entry    ┃ D.Pkts ┃ D.Bytes ┃ T.Pkts ┃ T.Bytes ┃ │ │       ┃ Entry    ┃ D.Pkts ┃ D.Bytes ┃ T.Pkts ┃ T.Bytes ┃ │
│ │       ┡━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │ │       ┡━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │
│ │       │ 0x105e9… │ 0      │ 0B      │ 0      │ 0B      │ │ │       │ 0x10669… │ 0      │ 0B      │ 0      │ 0B      │ │
│ │       │ 0x105e9… │ 0      │ 0B      │ 0      │ 0B      │ │ │       │ 0x10669… │ 0      │ 0B      │ 0      │ 0B      │ │
│ │       └──────────┴────────┴─────────┴────────┴─────────┘ │ │       └──────────┴────────┴─────────┴────────┴─────────┘ │
│ └── SAMPLE_PIPE (PIPE_BASIC)                               │ └── SAMPLE_PIPE (PIPE_BASIC)                               │
│     └── ┏━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │     └── ┏━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │
│         ┃ Entry    ┃ D.Pkts ┃ D.Bytes ┃ T.Pkts ┃ T.Bytes ┃ │         ┃ Entry    ┃ D.Pkts ┃ D.Bytes ┃ T.Pkts ┃ T.Bytes ┃ │
│         ┡━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │         ┡━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │
│         │ 0x105a9… │ 0      │ 0B      │ 0      │ 0B      │ │         │ 0x10629… │ 0      │ 0B      │ 0      │ 0B      │ │
│         │ 0x105a9… │ 0      │ 0B      │ 0      │ 0B      │ │         │ 0x10629… │ 0      │ 0B      │ 0      │ 0B      │ │
│         │ 0x105a9… │ 0      │ 0B      │ 0      │ 0B      │ │         │ 0x10629… │ 0      │ 0B      │ 0      │ 0B      │ │
│         │ 0x105a9… │ 0      │ 0B      │ 0      │ 0B      │ │         │ 0x10629… │ 0      │ 0B      │ 0      │ 0B      │ │
│         │ 0x105a9… │ 0      │ 0B      │ 0      │ 0B      │ │         │ 0x10629… │ 0      │ 0B      │ 0      │ 0B      │ │
│         └──────────┴────────┴─────────┴────────┴─────────┘ │         └──────────┴────────┴─────────┴────────┴─────────┘ │
└────────────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────┘
```