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

bool SimulatorClient::PrintMetrics() {
    matchmaking::MetricsRequest request;
    matchmaking::MetricsResponse response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetMetrics(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "GetMetrics RPC failed: " << status.error_message() << "\n";
        return false;
    }

    std::cout << "\n=== Matchmaking metrics ===\n";
    for (int i = 0; i < response.regions_size(); ++i) {
        const auto& rm = response.regions(i);
        std::cout << "Region " << rm.region()
                  << ": queue_size=" << rm.queue_size()
                  << ", matches=" << rm.matches() << "\n";
    }
    std::cout << "Last match: avg_mmr=" << response.last_match_average_mmr()
              << ", mmr_spread=" << response.last_match_mmr_spread()
              << ", avg_wait_s=" << response.last_match_average_wait_seconds()
              << "\n";

    return true;
}

bool SimulatorClient::PrintQueue() {
    matchmaking::MetricsRequest request;
    matchmaking::QueueSnapshot response;
    grpc::ClientContext context;

    grpc::Status status = stub_->GetQueue(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "GetQueue RPC failed: " << status.error_message() << "\n";
        return false;
    }

    std::cout << "\n=== Queue snapshot ===\n";
    for (int i = 0; i < response.players_size(); ++i) {
        const auto& qp = response.players(i);
        std::cout << "Player " << qp.id()
                  << " region=" << qp.region()
                  << " mmr=" << qp.mmr()
                  << " ping_na=" << qp.ping_na()
                  << " ping_eu=" << qp.ping_eu()
                  << " ping_asia=" << qp.ping_asia()
                  << " waited_s=" << qp.waited_seconds()
                  << "\n";
    }

    return true;
}

