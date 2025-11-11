#pragma once
#include <vector>
#include "matchmaker.pb.h"

class MatchBuilder {
public:
    static matchmaking::Match BuildBalancedMatch(std::vector<matchmaking::Player>& queue);

private:
    static void SortByMMR(std::vector<matchmaking::Player>& queue);
    static void SortByPing(std::vector<matchmaking::Player>& queue);
};
