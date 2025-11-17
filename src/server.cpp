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

Status MatchmakerServiceImpl::StreamMatches(ServerContext* context, const PlayerID* request, ServerWriter<Match>* writer) {
    const std::string player_id = request->id();

    while (!context->IsCancelled()) {
        auto matches = engine_.GetMatchesForPlayer(player_id);
        for (auto& m : matches) {
            writer->Write(m);
        }
        if (!matches.empty()) {
            return Status::OK;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return Status::OK;
}
