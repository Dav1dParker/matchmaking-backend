#include "MatchBuilder.h"
#include <algorithm>
#include <iostream>
using namespace matchmaking;

void MatchBuilder::SortByMMR(std::vector<Player>& queue) {
    std::sort(queue.begin(), queue.end(),
              [](const Player& a, const Player& b) { return a.mmr() > b.mmr(); });
}

void MatchBuilder::SortByPing(std::vector<Player>& queue) {
    std::sort(queue.begin(), queue.end(),
              [](const Player& a, const Player& b) { return a.ping() < b.ping(); });
}

Match MatchBuilder::BuildBalancedMatch(std::vector<Player>& queue) {
    if (queue.size() < 4)
        return {};

    SortByMMR(queue);

    Match match;
    match.set_match_id("match_" + std::to_string(rand()));

    // Simple balancing: highest + lowest + middle two
    *match.add_players() = queue.front();
    *match.add_players() = queue.back();
    queue.erase(queue.begin());
    queue.pop_back();

    *match.add_players() = queue.front();
    *match.add_players() = queue.back();
    queue.erase(queue.begin());
    queue.pop_back();

    std::cout << "Built balanced match by MMR.\n";
    return match;
}
