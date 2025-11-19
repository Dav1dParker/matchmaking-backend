#pragma once
#include <grpcpp/grpcpp.h>
#include "matchmaker.grpc.pb.h"
#include "Engine/Engine.h"

class MatchmakerServiceImpl final : public matchmaking::Matchmaker::Service {
public:
    MatchmakerServiceImpl();
    ~MatchmakerServiceImpl() override;

    grpc::Status Enqueue(grpc::ServerContext*,
                         const matchmaking::Player* request,
                         matchmaking::EnqueueResponse* response) override;

    grpc::Status Cancel(grpc::ServerContext*,
                        const matchmaking::PlayerID* request,
                        matchmaking::CancelResponse* response) override;

    grpc::Status StreamMatches(grpc::ServerContext*,
                               const matchmaking::PlayerID* request,
                               grpc::ServerWriter<matchmaking::Match>* writer) override;

    grpc::Status GetMetrics(grpc::ServerContext*,
                            const matchmaking::MetricsRequest* request,
                            matchmaking::MetricsResponse* response) override;

    grpc::Status GetQueue(grpc::ServerContext*,
                          const matchmaking::MetricsRequest* request,
                          matchmaking::QueueSnapshot* response) override;

private:
    Engine engine_;
};
