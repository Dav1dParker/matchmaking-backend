#pragma once
#include "matchmaker.pb.h"
#include "PlayerEntry.h"
#include "EngineConfig.h"

struct MatchMetrics {
    double average_mmr = 0.0;
    int min_mmr = 0;
    int max_mmr = 0;
    double average_wait_ms = 0.0;
};

class MatchBuilder {
public:
    static bool BuildMatch(std::deque<PlayerEntry>& queue,
                           matchmaking::Match& outMatch,
                           const EngineConfig& config,
                           const std::string& region,
                           MatchMetrics* metrics = nullptr);
};
