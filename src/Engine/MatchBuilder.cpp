#include "MatchBuilder.h"
#include <algorithm>

using namespace matchmaking;

bool MatchBuilder::BuildMatch(std::deque<PlayerEntry>& queue,
                              Match& outMatch)
{
    if (queue.size() < 10)
        return false;

    // Build a simple FIFO 10-player match for now
    outMatch.set_match_id("match_" + std::to_string(rand()));

    for (int i = 0; i < 10; i++) {
        *outMatch.add_players() = queue.front().player;
        queue.pop_front();
    }

    return true;
}
