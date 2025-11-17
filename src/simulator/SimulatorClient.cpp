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

bool SimulatorClient::StreamMatches(const std::string& player_id) {
    matchmaking::PlayerID request;
    request.set_id(player_id);

    grpc::ClientContext context;
    matchmaking::Match match;

    std::unique_ptr<grpc::ClientReader<matchmaking::Match>> reader =
        stub_->StreamMatches(&context, request);

    while (reader->Read(&match)) {
        std::cout << "Received match " << match.match_id()
                  << " for player " << player_id
                  << " with " << match.players_size() << " players\n";
    }

    grpc::Status status = reader->Finish();
    if (!status.ok()) {
        std::cerr << "StreamMatches RPC failed for " << player_id
                  << ": " << status.error_message() << "\n";
        return false;
    }

    return true;
}

