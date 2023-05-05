import grpc
import math

from rich.tree import Tree
from rich.table import Table
from rich.live import Live
from rich.console import Console
from time import sleep

from counter_spy_pb2 import EmptyRequest, Entry, Pipe, Port, QueryResult
from counter_spy_pb2_grpc import CounterSpyStub

def convert_size(size_bytes):
   if size_bytes == 0:
       return "0B"
   size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
   i = int(math.floor(math.log(size_bytes, 1024)))
   p = math.pow(1024, i)
   s = round(size_bytes / p, 2)
   return "%s %s" % (s, size_name[i])

def rich_port_stats(port: Port):
    port_tree = Tree(label="Pipes")
    for pipe in port.pipes:
        pipe_title = pipe.name #+ "("
#        if pipe.is_root():
#            pipe_title += "root, "
#        pipe_title += DocaFlowPipeType.Name(pipe.cfg.attr.type) + ")"
        pipe_tree = port_tree.add(pipe_title)
        entry_table = Table()
        [entry_table.add_column(c) for c in ["Entry", "T.Pkts", "T.Bytes"]]
        for entry in pipe.entries:
#            stats = entry.query()
            id = hex(entry.id)
            if len(id) > 16:
                id = id[0:6] + "..." + id[-7:-1]
            entry_table.add_row(id, 
                str(entry.packets), convert_size(entry.bytes))
        pipe_tree.add(entry_table)
    return port_tree

def show_rich_stats(result: QueryResult, stats_table : Table):
    stats_table.columns.clear()
    for port in result.ports:
        stats_table.add_column(f"Port {port.port_id}")
    
    stats_table.add_row(*[rich_port_stats(port) for port in result.ports])

channel = grpc.insecure_channel("127.0.0.1:50051")
stub = CounterSpyStub(channel)
stats_table = Table(show_header=True)
with Live(stats_table, refresh_per_second=4, screen=False) as live:
    while (True):
        try:
            result = stub.getFlowCounts(EmptyRequest())
            show_rich_stats(result, stats_table)
        except Exception as e:
            stats_table.columns.clear()
            #pass
            if hasattr(e, 'details'):
                print("Connecting..." + str(e.details()))
            else:
                print(e)
        sleep(1)

