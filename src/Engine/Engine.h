#pragma once
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "matchmaker.pb.h"
#include "PlayerEntry.h"
#include "EngineConfig.h"
#include "MatchPersistence.h"

struct EngineMetrics {
    std::unordered_map<std::string, std::size_t> queue_sizes_per_region;
    std::unordered_map<std::string, std::size_t> matches_per_region;
    double last_match_average_mmr = 0.0;
    double last_match_mmr_spread = 0.0;
    double last_match_average_wait_seconds = 0.0;
};

class Engine {
public:
    Engine();
    ~Engine();

    void Start();
    void Stop();

    void AddPlayer(const matchmaking::Player& player);
    bool RemovePlayer(const std::string& id);
    std::vector<matchmaking::Match> GetMatchesForPlayer(const std::string& id);

private:
    void TickLoop();

    std::deque<PlayerEntry> queue_;
    std::unordered_map<std::string, std::vector<matchmaking::Match>> pendingMatches_;
    EngineConfig config_;
    MatchPersistence persistence_;
    EngineMetrics metrics_;

    std::mutex mtx_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};
