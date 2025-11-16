#include "simulator/SimulatorClient.h"

#include <iostream>

SimulatorClient::SimulatorClient(const std::string& target_address) {
    auto channel = grpc::CreateChannel(
        target_address, grpc::InsecureChannelCredentials());

    stub_ = matchmaking::Matchmaker::NewStub(channel);
}

bool SimulatorClient::Enqueue(const matchmaking::Player& player) {
    matchmaking::EnqueueResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->Enqueue(&context, player, &response);

    if (!status.ok()) {
        std::cerr << "Enqueue RPC failed: " << status.error_message() << "\n";
        return false;
    }

    if (!response.success()) {
        std::cerr << "Enqueue RPC returned success=false\n";
        return false;
    }

    return true;
}

