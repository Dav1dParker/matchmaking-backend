cmake_minimum_required(VERSION 3.20)
project(MatchmakingBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

include_directories(generated src)

add_executable(matchmaker_server
        src/server.cpp
        generated/matchmaker.pb.cc
        generated/matchmaker.grpc.pb.cc
)

target_link_libraries(matchmaker_server PRIVATE
        gRPC::grpc++
        protobuf::libprotobuf
)
