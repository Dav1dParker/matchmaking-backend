#include "MatchBuilder.h"

#include <memory>
#include <vector>

using namespace matchmaking;

bool MatchBuilder::BuildMatch(std::deque<PlayerEntry>& queue,
                              Match& outMatch)
{
    if (queue.size() < 10)
        return false;

    const int seed_mmr = queue.front().player.mmr();
    const int window = 200;
    const int min_mmr = seed_mmr - window;
    const int max_mmr = seed_mmr + window;

    std::vector<std::size_t> eligible_indices;
    eligible_indices.reserve(queue.size());

    for (std::size_t i = 0; i < queue.size(); ++i) {
        int mmr = queue[i].player.mmr();
        if (mmr >= min_mmr && mmr <= max_mmr) {
            eligible_indices.push_back(i);
            if (eligible_indices.size() == 10)
                break;
        }
    }

    if (eligible_indices.size() < 10)
        return false;

    outMatch.set_match_id("match_" + std::to_string(rand()));

    std::vector<bool> selected(queue.size(), false);
    for (std::size_t j = 0; j < 10; ++j) {
        std::size_t idx = eligible_indices[j];
        selected[idx] = true;
        *outMatch.add_players() = queue[idx].player;
    }

    std::deque<PlayerEntry> remaining;

    for (std::size_t i = 0; i < queue.size(); ++i) {
        if (!selected[i]) {
            remaining.push_back(queue[i]);
        }
    }

    queue.swap(remaining);

    return true;
}
