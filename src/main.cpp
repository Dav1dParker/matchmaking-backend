#include <grpcpp/grpcpp.h>
#include "server.h"
#include "Engine/Engine.h"

int main() {
    std::string server_address("0.0.0.0:50051");
    MatchmakerServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Matchmaker server running on " << server_address << std::endl;
    server->Wait();

    return 0;
}
