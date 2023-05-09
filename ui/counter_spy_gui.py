import argparse
import grpc
import math

from rich.tree import Tree
from rich.table import Table
from rich.live import Live
from rich.console import Console
from time import sleep

from counter_spy_pb2 import EmptyRequest, Entry, Pipe, Port, PortType, QueryResult
from counter_spy_pb2_grpc import CounterSpyStub

def convert_size(size_bytes, unit='B'):
   if size_bytes == 0:
       return "0B"
   size_name = ("", "K", "M", "G", "T", "P", "E", "Z", "Y")
   i = int(math.floor(math.log(size_bytes, 1024)))
   p = math.pow(1024, i)
   s = round(size_bytes / p, 2)
   return "%s %s%s" % (s, size_name[i], unit)

def add_counter_row(entry_table, entry, id):
    if id is str and len(id) > 16:
        id = id[0:6] + "..." + id[-7:-1]
    entry_table.add_row(id, 
        convert_size(entry.delta_packets, 'P'), 
        convert_size(entry.delta_bytes),
        convert_size(entry.total_packets, 'P'), 
        convert_size(entry.total_bytes)
        )

def add_shared_counter_table(
        parent_tree: Tree, 
        table_title: str, 
        first_col: str,
        shared_counters: list):
    if len(shared_counters) == 0:
        return

    shared_ctr_tree = parent_tree.add(table_title)
    ctr_table = Table(first_col, "D.Pkts", "D.Bytes", "T.Pkts", "T.Bytes")
    for counter in shared_counters:
        add_counter_row(ctr_table, counter, str(counter.shared_counter_id))
    shared_ctr_tree.add(ctr_table)

def rich_port_stats(port: Port):
    port_tree = Tree(label="Pipes")
    for pipe in port.pipes:
        pipe_title = pipe.name + " Entries ("
        if pipe.is_root:
            pipe_title += "root, "
        pipe_title += PortType.Name(pipe.type) + ")"
        pipe_tree = port_tree.add(pipe_title)
        entry_table = Table("Entry", "D.Pkts", "D.Bytes", "T.Pkts", "T.Bytes")
        for entry in pipe.entries:
            add_counter_row(entry_table, entry, hex(entry.id))
        pipe_tree.add(entry_table)

        add_shared_counter_table(port_tree, pipe.name + " Shared Counters", "Sh.Counter", pipe.shared_counters)

    add_shared_counter_table(port_tree, "Port Shared Counters", "Sh.Counter", port.shared_counters)

    return port_tree

def show_rich_stats(result: QueryResult, stats_table : Table):
    stats_table.columns.clear()
    for port in result.ports:
        stats_table.add_column(f"Port {port.port_id}")
    
    stats_table.add_row(*[rich_port_stats(port) for port in result.ports])

parser = argparse.ArgumentParser(
    prog='counter_spy_gui', 
    description="Displays DOCA flow coutners in a remotely executing program wrapped with counter-spy")
parser.add_argument('-r', '--remote', 
    help="the remote address running counter-spy", 
    default="127.0.0.1")
args = parser.parse_args()

channel = grpc.insecure_channel(args.remote + ":50051")
stub = CounterSpyStub(channel)
stats_table = Table(show_header=True)
with Live(stats_table, refresh_per_second=4, screen=False) as live:
    while (True):
        try:
            result = stub.getFlowCounts(EmptyRequest())
            show_rich_stats(result, stats_table)
        except Exception as e:
            stats_table.columns.clear()
            stats_table.add_column("Error")
            if hasattr(e, 'details'):
                err = e.details()
            else:
                err = str(e)
            stats_table.add_row(err)
            
        sleep(1)

