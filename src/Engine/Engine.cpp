#include "Engine/Engine.h"
#include <chrono>
#include <iostream>

#include "MatchBuilder.h"
using namespace matchmaking;

Engine::Engine() = default;
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
    regionQueues_[player.region()].push_back(PlayerEntry(player));
}

bool Engine::RemovePlayer(const std::string& id) {
    std::scoped_lock lock(mtx_);
    for (auto& [region, queue] : regionQueues_) {
        auto it = std::remove_if(queue.begin(), queue.end(),
                                 [&](const PlayerEntry& e) {
                                     return e.player.id() == id;
                                 });

        if (it != queue.end()) {
            queue.erase(it, queue.end());
            return true;
        }
    }
    return false;
}

std::vector<Match> Engine::GetNewMatches() {
    std::scoped_lock lock(mtx_);
    auto result = newMatches_;
    newMatches_.clear();
    return result;
}

void Engine::TickLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::scoped_lock lock(mtx_);

        for (auto& [region, queue] : regionQueues_) {
            matchmaking::Match match;

            while (true) {
                if (MatchBuilder::BuildMatch(queue, match)) {
                    std::cout << "Created match " << match.match_id()
                              << " in region " << region
                              << " with " << match.players_size()
                              << " players" << std::endl;
                    newMatches_.push_back(match);
                    match = matchmaking::Match();  // reset for next
                } else {
                    break;
                }
            }

        }
    }
}

Match Engine::TryBuildMatch(std::vector<Player>& queue) {
    Match match;
    match.set_match_id("match_" + std::to_string(rand()));
    for (int i = 0; i < 4; ++i) {
        *match.add_players() = queue.front();
        queue.erase(queue.begin());
    }
    std::cout << "Built match with 4 players.\n";
    return match;
}
