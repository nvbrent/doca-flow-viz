#!/bin/sh
python3 -m grpc_tools.protoc \
    -I../protos \
    --python_out=. --grpc_python_out=. \
    counter_spy.proto
