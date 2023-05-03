import grpc
from time import sleep
from counter_spy_pb2 import EmptyRequest
from counter_spy_pb2_grpc import CounterSpyStub

channel = grpc.insecure_channel("127.0.0.1:51999")
stub = CounterSpyStub(channel)
while (True):
    sleep(1)
    try:
        result = stub.getFlowCounts(EmptyRequest())
        print(str(result))
    except:
        print("Not connected...")

