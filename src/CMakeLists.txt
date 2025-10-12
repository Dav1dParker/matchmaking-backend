set(SOURCES
        main.cpp
)

add_executable(matchmaker ${SOURCES})
target_link_libraries(matchmaker PRIVATE gRPC::grpc++ protobuf::libprotobuf)
