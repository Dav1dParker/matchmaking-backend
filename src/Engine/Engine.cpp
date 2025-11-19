#include "Engine/Engine.h"
#include <chrono>
#include <iostream>

#include "MatchBuilder.h"
using namespace matchmaking;

Engine::Engine()
    : config_(EngineConfig::LoadFromFile("config/server_config.json")),
      persistence_(config_.matches_path) {}
Engine::~Engine() { Stop(); }

void Engine::Start() {
    running_ = true;
    worker_ = std::thread(&Engine::TickLoop, this);
}

void Engine::Stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

void Engine::AddPlayer(const Player& player) {
    std::scoped_lock lock(mtx_);
    queue_.push_back(PlayerEntry(player));
    std::cout << "Players currently in queue: " << queue_.size() << std::endl;
}

bool Engine::RemovePlayer(const std::string& id) {
    std::scoped_lock lock(mtx_);
    auto it = std::remove_if(queue_.begin(), queue_.end(),
                             [&](const PlayerEntry& e) {
                                 return e.player.id() == id;
                             });

    if (it != queue_.end()) {
        queue_.erase(it, queue_.end());
        return true;
    }
    return false;
}

std::vector<Match> Engine::GetMatchesForPlayer(const std::string& id) {
    std::scoped_lock lock(mtx_);
    auto it = pendingMatches_.find(id);
    if (it == pendingMatches_.end()) {
        return {};
    }
    auto result = it->second;
    pendingMatches_.erase(it);
    return result;
}

EngineMetrics Engine::GetMetricsSnapshot() const {
    std::scoped_lock lock(mtx_);
    return metrics_;
}

void Engine::FillQueueSnapshot(matchmaking::QueueSnapshot& snapshot) const {
    std::scoped_lock lock(mtx_);
    auto now = std::chrono::steady_clock::now();
    for (const auto& entry : queue_) {
        auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - entry.queuedAt).count();
        if (waited_ms < 0) {
            waited_ms = 0;
        }
        auto* qp = snapshot.add_players();
        qp->set_id(entry.player.id());
        qp->set_region(entry.player.region());
        qp->set_mmr(entry.player.mmr());
        qp->set_ping_na(entry.player.ping_na());
        qp->set_ping_eu(entry.player.ping_eu());
        qp->set_ping_asia(entry.player.ping_asia());
        qp->set_waited_seconds(static_cast<double>(waited_ms) / 1000.0);
    }
}

void Engine::TickLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.tick_interval_ms));
        std::scoped_lock lock(mtx_);

        const std::string regions[] = {"NA", "EU", "ASIA"};
        for (const auto& region : regions) {
            matchmaking::Match match;

            while (true) {
                MatchMetrics metrics;
                if (MatchBuilder::BuildMatch(queue_, match, config_, region, &metrics)) {
                    double mmr_spread = static_cast<double>(metrics.max_mmr - metrics.min_mmr);
                    double avg_wait_seconds = metrics.average_wait_ms / 1000.0;
                    std::cout << "Created match " << match.match_id()
                              << " in region " << region
                              << " with " << match.players_size()
                              << " players"
                              << " avg_mmr=" << metrics.average_mmr
                              << " mmr_spread=" << mmr_spread
                              << " avg_wait_s=" << avg_wait_seconds
                              << std::endl;
                    metrics_.matches_per_region[region] += 1;
                    metrics_.last_match_average_mmr = metrics.average_mmr;
                    metrics_.last_match_mmr_spread = mmr_spread;
                    metrics_.last_match_average_wait_seconds = avg_wait_seconds;
                    for (int i = 0; i < match.players_size(); ++i) {
                        const auto& p = match.players(i);
                        pendingMatches_[p.id()].push_back(match);
                    }
                    persistence_.Append(match);
                    match = matchmaking::Match();  // reset for next
                } else {
                    break;
                }
            }
        }

        metrics_.queue_sizes_per_region.clear();
        for (const auto& entry : queue_) {
            const std::string& region = entry.player.region();
            metrics_.queue_sizes_per_region[region] += 1;
        }
    }
}
