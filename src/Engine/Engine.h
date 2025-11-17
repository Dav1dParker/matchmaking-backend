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

    std::unordered_map<std::string, std::deque<PlayerEntry>> regionQueues_;
    std::unordered_map<std::string, std::vector<matchmaking::Match>> pendingMatches_;
    EngineConfig config_;
    MatchPersistence persistence_;

    std::mutex mtx_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};
