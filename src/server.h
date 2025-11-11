#pragma once
#include <grpcpp/grpcpp.h>
#include "matchmaker.grpc.pb.h"
#include "engine/Engine.h"

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

private:
    Engine engine_;
};
