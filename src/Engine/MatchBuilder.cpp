#include "MatchBuilder.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <vector>

using namespace matchmaking;

bool MatchBuilder::BuildMatch(std::deque<PlayerEntry>& queue,
                              Match& outMatch,
                              const EngineConfig& config)
{
    if (queue.size() < 10)
        return false;

    auto now = std::chrono::steady_clock::now();
    const PlayerEntry& seed_entry = queue.front();
    auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - seed_entry.queuedAt).count();
    if (waited_ms < 0) {
        waited_ms = 0;
    }

    int waited_seconds = static_cast<int>(waited_ms / 1000);
    const int base_window = 200;
    const int per_second = 50;
    int window = base_window + per_second * waited_seconds;
    const int max_window = 1000;
    if (window > max_window) {
        window = max_window;
    }

    const int seed_mmr = seed_entry.player.mmr();
    const int min_mmr = seed_mmr - window;
    const int max_mmr = seed_mmr + window;

    int ping_window = config.max_ping_ms +
                      config.ping_relax_per_second * waited_seconds;
    if (ping_window > config.max_ping_ms_cap) {
        ping_window = config.max_ping_ms_cap;
    }

    std::vector<std::size_t> eligible_indices;
    eligible_indices.reserve(queue.size());

    for (std::size_t i = 0; i < queue.size(); ++i) {
        int mmr = queue[i].player.mmr();
        int ping = queue[i].player.ping();
        if (mmr >= min_mmr && mmr <= max_mmr &&
            ping <= ping_window) {
            eligible_indices.push_back(i);
            if (eligible_indices.size() == 10)
                break;
        }
    }

    if (eligible_indices.size() < 10)
        return false;

    outMatch.set_match_id("match_" + std::to_string(rand()));

    struct Candidate {
        std::size_t index;
        int mmr;
    };

    std::vector<Candidate> candidates;
    candidates.reserve(10);
    for (std::size_t j = 0; j < 10; ++j) {
        std::size_t idx = eligible_indices[j];
        candidates.push_back(Candidate{idx, queue[idx].player.mmr()});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.mmr > b.mmr;
              });

    std::vector<std::size_t> team_a;
    std::vector<std::size_t> team_b;
    team_a.reserve(5);
    team_b.reserve(5);
    int sum_a = 0;
    int sum_b = 0;

    for (const auto& c : candidates) {
        bool choose_a = false;
        if (team_a.size() < 5 && team_b.size() < 5) {
            choose_a = sum_a <= sum_b;
        } else if (team_a.size() < 5) {
            choose_a = true;
        } else {
            choose_a = false;
        }

        if (choose_a) {
            team_a.push_back(c.index);
            sum_a += c.mmr;
        } else {
            team_b.push_back(c.index);
            sum_b += c.mmr;
        }
    }

    std::vector<bool> selected(queue.size(), false);
    for (std::size_t idx : team_a) {
        selected[idx] = true;
        *outMatch.add_players() = queue[idx].player;
    }
    for (std::size_t idx : team_b) {
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
