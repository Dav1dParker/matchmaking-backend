#include "server.h"

using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using namespace matchmaking;

MatchmakerServiceImpl::MatchmakerServiceImpl() {
    engine_.Start();
}

MatchmakerServiceImpl::~MatchmakerServiceImpl() {
    engine_.Stop();
}

Status MatchmakerServiceImpl::Enqueue(ServerContext*, const Player* request, EnqueueResponse* response) {
    engine_.AddPlayer(*request);
    response->set_success(true);
    return Status::OK;
}

Status MatchmakerServiceImpl::Cancel(ServerContext*, const PlayerID* request, CancelResponse* response) {
    response->set_success(engine_.RemovePlayer(request->id()));
    return Status::OK;
}

Status MatchmakerServiceImpl::StreamMatches(ServerContext* context, const PlayerID*, ServerWriter<Match>* writer) {
    while (!context->IsCancelled()) {
        auto matches = engine_.GetNewMatches();
        for (auto& m : matches)
            writer->Write(m);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return Status::OK;
}
