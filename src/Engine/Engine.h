#pragma once
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "matchmaker.pb.h"
#include "PlayerEntry.h"

class Engine {
public:
    Engine();
    ~Engine();

    void Start();
    void Stop();

    void AddPlayer(const matchmaking::Player& player);
    bool RemovePlayer(const std::string& id);
    std::vector<matchmaking::Match> GetNewMatches();

private:
    void TickLoop();

    std::unordered_map<std::string, std::deque<PlayerEntry>> regionQueues_;
    std::vector<matchmaking::Match> newMatches_;

    std::mutex mtx_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};
