#pragma once

#include <chrono>
#include "matchmaker.pb.h"

struct PlayerEntry {
    matchmaking::Player player;
    std::chrono::steady_clock::time_point queuedAt;

    explicit PlayerEntry(const matchmaking::Player& p)
        : player(p),
          queuedAt(std::chrono::steady_clock::now()) {}
};
