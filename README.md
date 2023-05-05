# DOCA Flow Counter-Spy

The `counter-spy` library is a wrapper for `DOCA Flow`. It intercepts calls to create/destroy Pipes, and keeps an internal database of counters which can be queried. It also listens on a gRPC connection and allows a client to retrieve the state of all the counters.

(`TODO`: shared counters; pipe miss-counters)

Prerequisites: DOCA 2.0.2

Building `counter-spy`:
```
meson build
ninja -C build
```

Usage:

Simply prefix your regular DOCA application command line with `LD_PRELOAD=/path/to/libcounter-spy.so` and run your DOCA program normally.

Example of running the built-in sample:
```
LD_PRELOADbuild/lib/libdoca-counter-spy.so build/test/sample_flow_program -a17:00.0,dv_flow_en=2 -a17:00.1,dv_flow_en=2 -c0x1
```

In another terminal (local or remote), run the UI to see updates:
```
python3 counter_spy_gui.py [-r host-running-doca]
```

The `-r` remote-host flag is optional; the default is localhost.

Note that the port number, 50051, is not presently configurable.

Example output:
```
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Port 0                                     ┃ Port 1                                     ┃
┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┩
│ Pipes                                      │ Pipes                                      │
│ ├── NEXT_PIPE (PIPE_BASIC)                 │ ├── NEXT_PIPE (PIPE_BASIC)                 │
│ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │
│ │       ┃ Entry       ┃ T.Pkts ┃ T.Bytes ┃ │ │       ┃ Entry       ┃ T.Pkts ┃ T.Bytes ┃ │
│ │       ┡━━━━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │ │       ┡━━━━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │
│ │       │ 0x105e97ec0 │ 0      │ 0B      │ │ │       │ 0x106697ec0 │ 0      │ 0B      │ │
│ │       │ 0x105e97f80 │ 0      │ 0B      │ │ │       │ 0x106697f80 │ 0      │ 0B      │ │
│ │       └─────────────┴────────┴─────────┘ │ │       └─────────────┴────────┴─────────┘ │
│ └── SAMPLE_PIPE (PIPE_BASIC)               │ └── SAMPLE_PIPE (PIPE_BASIC)               │
│     └── ┏━━━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │     └── ┏━━━━━━━━━━━━━┳━━━━━━━━┳━━━━━━━━━┓ │
│         ┃ Entry       ┃ T.Pkts ┃ T.Bytes ┃ │         ┃ Entry       ┃ T.Pkts ┃ T.Bytes ┃ │
│         ┡━━━━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │         ┡━━━━━━━━━━━━━╇━━━━━━━━╇━━━━━━━━━┩ │
│         │ 0x105a96c00 │ 0      │ 0B      │ │         │ 0x106297c80 │ 0      │ 0B      │ │
│         │ 0x105a96cc0 │ 0      │ 0B      │ │         │ 0x106297d40 │ 0      │ 0B      │ │
│         │ 0x105a96d80 │ 0      │ 0B      │ │         │ 0x106297e00 │ 0      │ 0B      │ │
│         │ 0x105a96e40 │ 0      │ 0B      │ │         │ 0x106297ec0 │ 0      │ 0B      │ │
│         │ 0x105a96f00 │ 0      │ 0B      │ │         │ 0x106297f80 │ 0      │ 0B      │ │
│         └─────────────┴────────┴─────────┘ │         └─────────────┴────────┴─────────┘ │
└────────────────────────────────────────────┴────────────────────────────────────────────┘
```