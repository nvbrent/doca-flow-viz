#include <memory>
#include <thread>

#include <doca_flow.h>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <counter_spy_c.h>
#include <counter_spy.h>
#include <counter_spy_svc.h>

std::unique_ptr<CounterSpyServiceImpl> service_impl;
std::unique_ptr<grpc::Server> server;
std::thread server_thread;

void CounterSpyServiceImpl::addStats(
    const EntryFlowStats &entry_stats,
    doca_flow_counter_spy::Entry *entry_obj)
{
    if (entry_stats.entry_ptr) {
        entry_obj->set_id((intptr_t)entry_stats.entry_ptr);
    } else if (entry_stats.shared_counter_id) {
        entry_obj->set_shared_counter_id(entry_stats.shared_counter_id);
    }
    entry_obj->set_total_bytes(entry_stats.total.total_bytes);
    entry_obj->set_total_packets(entry_stats.total.total_pkts);
    entry_obj->set_delta_bytes(entry_stats.delta.total_bytes);
    entry_obj->set_delta_packets(entry_stats.delta.total_pkts);
}

::grpc::Status 
CounterSpyServiceImpl::getFlowCounts(
        ::grpc::ServerContext* context, 
        const ::doca_flow_counter_spy::EmptyRequest* request, 
        ::doca_flow_counter_spy::QueryResult* response)
{
    for (auto &port_iter : ports) {
        auto &port_counters = port_iter.second;
        auto *port_obj = response->add_ports();
        port_obj->set_port_id(port_counters.port_id());

        auto port_stats = port_counters.query();
        for (auto &pipe_iter : port_stats.port_stats) {
            auto &pipe = pipe_iter.second;
            auto *pipe_obj = port_obj->add_pipes();
            pipe_obj->set_name(pipe_iter.first);
            pipe_obj->set_is_root(pipe.pipe_mon->is_root());
            pipe_obj->set_type(
                static_cast<doca_flow_counter_spy::PortType>(pipe.pipe_mon->type()));

            for (auto &entry_iter : pipe_iter.second.pipe_stats) {
                addStats(entry_iter, pipe_obj->add_entries());
            }

            for (auto &shared_iter : pipe_iter.second.pipe_shared_counters) {
                addStats(shared_iter, pipe_obj->add_shared_counters());
            }
        }

        for (auto &shared_iter : port_stats.port_shared_counters) {
            addStats(shared_iter, port_obj->add_shared_counters());
        }
    }
    return ::grpc::Status::OK;
}

void counter_spy_start_service(void)
{
    const char * fake_stats_var = secure_getenv(COUNTER_SPY_FAKE_STATS_ENV_VAR.c_str());
    if (fake_stats_var) {
        printf("DOCA Flow Counter Spy: Enabling fake stats.\n");
        fake_stats_enabled = true;
    }

    std::string server_address("0.0.0.0:50051");
    service_impl = std::make_unique<CounterSpyServiceImpl>();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service_impl.get());
    server = builder.BuildAndStart();
    printf("DOCA Flow Counter Spy: Listening on %s\n", server_address.c_str());
    server_thread = std::thread([](){
        server->Wait();
        printf("DOCA Flow Counter Spy: server stopped\n");
        server.reset();
    });
}

void counter_spy_stop_service(void)
{
    if (server) {
        printf("DOCA Flow Counter Spy: shutting down...\n");
        server->Shutdown();
        server.reset();
    }
    if (server_thread.joinable()) {
        server_thread.join();
    }
}
