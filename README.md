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
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Port 0                                                               ┃ Port 1                                                               ┃
┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┩
│ Pipes                                                                │ Pipes                                                                │
│ ├── NEXT_PIPE (PIPE_BASIC)                                           │ ├── NEXT_PIPE (PIPE_BASIC)                                           │
│ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓  │ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓    │
│ │       ┃ Entry       ┃ D.Pkts  ┃ D.Bytes   ┃ T.Pkts  ┃ T.Bytes   ┃  │ │       ┃ Entry       ┃ D.Pkts  ┃ D.Bytes ┃ T.Pkts  ┃ T.Bytes   ┃    │
│ │       ┡━━━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩  │ │       ┡━━━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩    │
│ │       │ 0x105e97ec0 │ 744.0 P │ 2.26 MB   │ 1.41 MP │ 151.02 MB │  │ │       │ 0x106697ec0 │ 1.73 KP │ 1.45 MB │ 1.38 MP │ 135.8 MB  │    │
│ │       │ 0x105e97f80 │ 3.93 KP │ 625.14 KB │ 1.52 MP │ 142.34 MB │  │ │       │ 0x106697f80 │ 971.0 P │ 1.11 MB │ 1.36 MP │ 141.99 MB │    │
│ │       └─────────────┴─────────┴───────────┴─────────┴───────────┘  │ │       └─────────────┴─────────┴─────────┴─────────┴───────────┘    
┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Port 0                                                               ┃ Port 1                                                               ┃
┡━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┩
│ Pipes                                                                │ Pipes                                                                │
│ ├── NEXT_PIPE (PIPE_BASIC)                                           │ ├── NEXT_PIPE (PIPE_BASIC)                                           │
│ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓ │ │   └── ┏━━━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓  │
│ │       ┃ Entry       ┃ D.Pkts   ┃ D.Bytes   ┃ T.Pkts  ┃ T.Bytes   ┃ │ │       ┃ Entry       ┃ D.Pkts  ┃ D.Bytes   ┃ T.Pkts  ┃ T.Bytes   ┃  │
│ │       ┡━━━━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩ │ │       ┡━━━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩  │
│ │       │ 0x105e97ec0 │ 19.92 KP │ 754.27 KB │ 1.65 MP │ 175.51 MB │ │ │       │ 0x106697ec0 │ 4.79 KP │ 2.37 MB   │ 1.66 MP │ 158.99 MB │  │
│ │       │ 0x105e97f80 │ 165.0 P  │ 710.23 KB │ 1.77 MP │ 162.44 MB │ │ │       │ 0x106697f80 │ 1.15 KP │ 624.06 KB │ 1.55 MP │ 163.23 MB │  │
│ │       └─────────────┴──────────┴───────────┴─────────┴───────────┘ │ │       └─────────────┴─────────┴───────────┴─────────┴───────────┘  │
│ └── SAMPLE_PIPE (PIPE_BASIC)                                         │ └── SAMPLE_PIPE (PIPE_BASIC)                                         │
│     └── ┏━━━━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓ │     └── ┏━━━━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━━┓ │
│         ┃ Entry       ┃ D.Pkts   ┃ D.Bytes   ┃ T.Pkts  ┃ T.Bytes   ┃ │         ┃ Entry       ┃ D.Pkts   ┃ D.Bytes   ┃ T.Pkts  ┃ T.Bytes   ┃ │
│         ┡━━━━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩ │         ┡━━━━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━╇━━━━━━━━━━━┩ │
│         │ 0x105a96c00 │ 11.49 KP │ 505.44 KB │ 1.7 MP  │ 170.07 MB │ │         │ 0x106297c80 │ 12.38 KP │ 788.32 KB │ 1.54 MP │ 167.88 MB │ │
│         │ 0x105a96cc0 │ 20.5 KP  │ 770.57 KB │ 1.65 MP │ 165.42 MB │ │         │ 0x106297d40 │ 569.0 P  │ 149.34 KB │ 1.62 MP │ 155.63 MB │ │
│         │ 0x105a96d80 │ 1.42 KP  │ 5.68 KB   │ 1.65 MP │ 173.12 MB │ │         │ 0x106297e00 │ 7.14 KP  │ 1.57 MB   │ 1.63 MP │ 169.31 MB │ │
│         │ 0x105a96e40 │ 2.9 KP   │ 250.19 KB │ 1.62 MP │ 152.56 MB │ │         │ 0x106297ec0 │ 10.75 KP │ 540.94 KB │ 1.66 MP │ 194.05 MB │ │
│         │ 0x105a96f00 │ 9.66 KP  │ 305.79 KB │ 1.72 MP │ 159.82 MB │ │         │ 0x106297f80 │ 8.09 KP  │ 40.88 KB  │ 1.51 MP │ 161.18 MB │ │
│         └─────────────┴──────────┴───────────┴─────────┴───────────┘ │         └─────────────┴──────────┴───────────┴─────────┴───────────┘ │
└──────────────────────────────────────────────────────────────────────┴──────────────────────────────────────────────────────────────────────┘
```