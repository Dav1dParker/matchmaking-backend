#pragma once
#include <vector>
#include "matchmaker.pb.h"
#include "PlayerEntry.h"

class MatchBuilder {
public:
    static bool BuildMatch(std::deque<PlayerEntry>& queue,
                           matchmaking::Match& outMatch);

private:
    static void SortByMMR(std::vector<matchmaking::Player>& queue);
    static void SortByPing(std::vector<matchmaking::Player>& queue);
};
