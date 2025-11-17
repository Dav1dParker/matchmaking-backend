#pragma once
#include "matchmaker.pb.h"
#include "PlayerEntry.h"
#include "EngineConfig.h"

class MatchBuilder {
public:
    static bool BuildMatch(std::deque<PlayerEntry>& queue,
                           matchmaking::Match& outMatch,
                           const EngineConfig& config);
};
