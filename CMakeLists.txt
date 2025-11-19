cmake_minimum_required(VERSION 3.20)
project(MatchmakingBackend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

enable_testing()

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_FILES ${PROTO_SRC_DIR}/matchmaker.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

add_custom_command(
        OUTPUT
            ${GENERATED_DIR}/matchmaker.pb.cc
            ${GENERATED_DIR}/matchmaker.pb.h
            ${GENERATED_DIR}/matchmaker.grpc.pb.cc
            ${GENERATED_DIR}/matchmaker.grpc.pb.h
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS
            --grpc_out=${GENERATED_DIR}
            --cpp_out=${GENERATED_DIR}
            -I ${PROTO_SRC_DIR}
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        VERBATIM
)

add_library(matchmaker_proto STATIC
        ${GENERATED_DIR}/matchmaker.pb.cc
        ${GENERATED_DIR}/matchmaker.grpc.pb.cc
)

target_include_directories(matchmaker_proto PUBLIC
        ${GENERATED_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(matchmaker_proto PUBLIC
        gRPC::grpc++
        protobuf::libprotobuf
)

add_executable(matchmaker_server
        src/main.cpp
        src/server.cpp
        src/server.h
        src/Engine/Engine.cpp
        src/Engine/Engine.h
        src/Engine/EngineConfig.cpp
        src/Engine/EngineConfig.h
        src/Engine/MatchBuilder.cpp
        src/Engine/MatchBuilder.h
        src/Engine/MatchPersistence.cpp
        src/Engine/MatchPersistence.h
)

target_link_libraries(matchmaker_server PRIVATE
        matchmaker_proto
)

add_executable(match_simulator
        src/simulator/main_simulator.cpp
        src/simulator/SimulatorClient.cpp
        src/simulator/SimConfig.cpp
        src/simulator/SimConfig.h
)

target_link_libraries(match_simulator PRIVATE
        matchmaker_proto
)

add_executable(matchmaking_tests
        tests/MatchBuilderTests.cpp
        src/Engine/MatchBuilder.cpp
        src/Engine/MatchBuilder.h
        src/Engine/PlayerEntry.h
)

target_link_libraries(matchmaking_tests PRIVATE
        matchmaker_proto
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME matchmaking_tests COMMAND matchmaking_tests)
