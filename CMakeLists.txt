cmake_minimum_required(VERSION 3.20)
project(MatchmakingBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# include directories
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(matchmaker_server
        src/main.cpp
        src/server.cpp
        src/server.h
        src/engine/Engine.cpp
        src/engine/Engine.h
        src/engine/MatchBuilder.cpp
        src/engine/MatchBuilder.h
        generated/matchmaker.pb.cc
        generated/matchmaker.grpc.pb.cc
)


target_link_libraries(matchmaker_server PRIVATE
        gRPC::grpc++
        protobuf::libprotobuf
)

add_executable(match_simulator
        src/simulator/main_simulator.cpp
        src/simulator/SimulatorClient.cpp
        generated/matchmaker.pb.cc
        generated/matchmaker.grpc.pb.cc
)

target_link_libraries(match_simulator PRIVATE
        gRPC::grpc++
        protobuf::libprotobuf
)
