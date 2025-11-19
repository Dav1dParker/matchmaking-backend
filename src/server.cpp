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

Status MatchmakerServiceImpl::GetMetrics(ServerContext*, const matchmaking::MetricsRequest*, matchmaking::MetricsResponse* response) {
    EngineMetrics snapshot = engine_.GetMetricsSnapshot();

    const std::string regions[] = {"NA", "EU", "ASIA"};
    for (const auto& region : regions) {
        matchmaking::RegionMetrics* rm = response->add_regions();
        rm->set_region(region);

        auto it_q = snapshot.queue_sizes_per_region.find(region);
        if (it_q != snapshot.queue_sizes_per_region.end()) {
            rm->set_queue_size(static_cast<uint64_t>(it_q->second));
        } else {
            rm->set_queue_size(0);
        }

        auto it_m = snapshot.matches_per_region.find(region);
        if (it_m != snapshot.matches_per_region.end()) {
            rm->set_matches(static_cast<uint64_t>(it_m->second));
        } else {
            rm->set_matches(0);
        }
    }

    response->set_last_match_average_mmr(snapshot.last_match_average_mmr);
    response->set_last_match_mmr_spread(snapshot.last_match_mmr_spread);
    response->set_last_match_average_wait_seconds(snapshot.last_match_average_wait_seconds);

    return Status::OK;
}

Status MatchmakerServiceImpl::GetQueue(ServerContext*, const matchmaking::MetricsRequest*, matchmaking::QueueSnapshot* response) {
    engine_.FillQueueSnapshot(*response);
    return Status::OK;
}
