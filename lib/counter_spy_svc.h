#pragma once

#include <counter_spy.pb.h>
#include <counter_spy.grpc.pb.h>
#include <counter_spy.h>

class CounterSpyServiceImpl : public doca_flow_counter_spy::CounterSpy::Service
{
public:
    ::grpc::Status getFlowCounts(
        ::grpc::ServerContext* context, 
        const ::doca_flow_counter_spy::EmptyRequest* request, 
        ::doca_flow_counter_spy::QueryResult* response) override;

    void addStats(
        const EntryFlowStats &entry_stats,
        doca_flow_counter_spy::Entry *entry_obj);
};
