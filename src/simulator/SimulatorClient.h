#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "matchmaker.grpc.pb.h"

class SimulatorClient {
public:
    explicit SimulatorClient(const std::string& target_address);

    bool Enqueue(const matchmaking::Player& player);
    bool StreamMatches(const std::string& player_id);
    bool PrintMetrics();
    bool PrintQueue();

private:
    std::unique_ptr<matchmaking::Matchmaker::Stub> stub_;
};

