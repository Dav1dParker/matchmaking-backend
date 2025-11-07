#include <string>
#include <grpcpp/grpcpp.h>
#include "matchmaker.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace matchmaking;

class MatchmakerServiceImpl final : public Matchmaker::Service {
    Status Enqueue(ServerContext*, const Player*, EnqueueResponse*) override {
        return Status::OK;
    }
    Status Cancel(ServerContext*, const PlayerID*, CancelResponse*) override {
        return Status::OK;
    }
    Status StreamMatches(ServerContext*, const PlayerID*, grpc::ServerWriter<Match>*) override {
        return Status::OK;
    }
};

int main() {
    std::string address = "0.0.0.0:50051";
    MatchmakerServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    //std::cout << "Matchmaker gRPC server running on 0.0.0.0:50051\n";
    server->Wait();
    return 0;
}
