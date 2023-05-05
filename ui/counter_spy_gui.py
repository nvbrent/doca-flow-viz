import grpc
from time import sleep
from counter_spy_pb2 import EmptyRequest
from counter_spy_pb2_grpc import CounterSpyStub

channel = grpc.insecure_channel("127.0.0.1:50051")
stub = CounterSpyStub(channel)
while (True):
    sleep(1)
    try:
        result = stub.getFlowCounts(EmptyRequest())
        s = str(result)
        print(s)
    except Exception as e:
        print("Connecting..." + str(e.details()))

