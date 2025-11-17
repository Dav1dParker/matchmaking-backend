#include "MatchBuilder.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

using namespace matchmaking;

namespace {

int GetRegionPing(const Player& p, const std::string& region) {
    if (region == "NA") {
        if (p.ping_na() > 0) return p.ping_na();
    } else if (region == "EU") {
        if (p.ping_eu() > 0) return p.ping_eu();
    } else if (region == "ASIA") {
        if (p.ping_asia() > 0) return p.ping_asia();
    }
    return p.ping();
}

bool IsRegionAllowedForPlayer(const PlayerEntry& entry,
                              const std::string& region,
                              long long waited_ms,
                              const EngineConfig& config) {
    struct RegionPing {
        const char* name;
        int ping;
    };

    const Player& p = entry.player;
    RegionPing regions[3] = {
        {"NA", GetRegionPing(p, "NA")},
        {"EU", GetRegionPing(p, "EU")},
        {"ASIA", GetRegionPing(p, "ASIA")}
    };

    std::sort(std::begin(regions), std::end(regions),
              [](const RegionPing& a, const RegionPing& b) {
                  return a.ping < b.ping;
              });

    int rank = -1;
    int region_ping = 0;
    for (int i = 0; i < 3; ++i) {
        if (region == regions[i].name) {
            rank = i;
            region_ping = regions[i].ping;
            break;
        }
    }

    if (rank == -1) {
        return false;
    }

    if (rank == 0) {
        return true;
    }

    if (region_ping < config.good_region_ping_ms) {
        return true;
    }

    long long required_ms = static_cast<long long>(rank) * config.cross_region_step_ms;
    return waited_ms >= required_ms;
}

}  // namespace

bool MatchBuilder::BuildMatch(std::deque<PlayerEntry>& queue,
                              Match& outMatch,
                              const EngineConfig& config,
                              const std::string& region,
                              MatchMetrics* metrics)
{
    if (queue.size() < 10)
        return false;

    auto now = std::chrono::steady_clock::now();

    std::size_t seed_index = queue.size();
    long long waited_ms = 0;

    for (std::size_t i = 0; i < queue.size(); ++i) {
        auto w = std::chrono::duration_cast<std::chrono::milliseconds>(now - queue[i].queuedAt).count();
        if (w < 0) {
            w = 0;
        }
        if (IsRegionAllowedForPlayer(queue[i], region, w, config)) {
            seed_index = i;
            waited_ms = w;
            break;
        }
    }

    if (seed_index == queue.size()) {
        return false;
    }

    const PlayerEntry& seed_entry = queue[seed_index];
    if (waited_ms < 0) {
        waited_ms = 0;
    }

    int relax_seconds = 0;
    if (waited_ms > config.min_wait_before_match_ms) {
        relax_seconds = static_cast<int>((waited_ms - config.min_wait_before_match_ms) / 1000);
    }

    int window = config.base_mmr_window +
                 config.mmr_relax_per_second * relax_seconds;
    if (window > config.max_mmr_window) {
        window = config.max_mmr_window;
    }

    const int seed_mmr = seed_entry.player.mmr();
    const int min_mmr = seed_mmr - window;
    const int max_mmr = seed_mmr + window;

    int ping_window = config.max_ping_ms +
                      config.ping_relax_per_second * relax_seconds;
    if (ping_window > config.max_ping_ms_cap) {
        ping_window = config.max_ping_ms_cap;
    }

    std::vector<std::size_t> eligible_indices;
    eligible_indices.reserve(queue.size());

    for (std::size_t i = 0; i < queue.size(); ++i) {
        auto wait_i = std::chrono::duration_cast<std::chrono::milliseconds>(now - queue[i].queuedAt).count();
        if (wait_i < 0) {
            wait_i = 0;
        }
        if (!IsRegionAllowedForPlayer(queue[i], region, wait_i, config)) {
            continue;
        }

        int mmr = queue[i].player.mmr();
        int ping = GetRegionPing(queue[i].player, region);
        if (mmr >= min_mmr && mmr <= max_mmr &&
            ping <= ping_window) {
            eligible_indices.push_back(i);
        }
    }

    if (eligible_indices.size() < 10)
        return false;

    outMatch.set_match_id("match_" + std::to_string(rand()));

    struct Candidate {
        std::size_t index;
        int mmr;
    };

    std::vector<Candidate> all_candidates;
    all_candidates.reserve(eligible_indices.size());
    for (std::size_t idx : eligible_indices) {
        all_candidates.push_back(Candidate{idx, queue[idx].player.mmr()});
    }

    std::sort(all_candidates.begin(), all_candidates.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.mmr < b.mmr;
              });

    int best_start = 0;
    int best_spread = std::numeric_limits<int>::max();
    for (std::size_t i = 0; i + 9 < all_candidates.size(); ++i) {
        int spread = all_candidates[i + 9].mmr - all_candidates[i].mmr;
        if (spread < best_spread) {
            best_spread = spread;
            best_start = static_cast<int>(i);
        }
    }

    int allowed_spread = config.max_allowed_mmr_diff;
    if (waited_ms > config.min_wait_before_match_ms) {
        allowed_spread += config.mmr_diff_relax_per_second * relax_seconds;
        if (allowed_spread > config.max_relaxed_mmr_diff) {
            allowed_spread = config.max_relaxed_mmr_diff;
        }
    }

    if (best_spread > allowed_spread) {
        return false;
    }

    std::vector<Candidate> candidates;
    candidates.reserve(10);
    for (int j = 0; j < 10; ++j) {
        candidates.push_back(all_candidates[best_start + j]);
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

    long long total_wait_ms = 0;
    long long sum_mmr_match = 0;
    int min_mmr_match = std::numeric_limits<int>::max();
    int max_mmr_match = std::numeric_limits<int>::min();
    int selected_count = 0;

    for (std::size_t idx : team_a) {
        selected[idx] = true;
        *outMatch.add_players() = queue[idx].player;

        int mmr = queue[idx].player.mmr();
        sum_mmr_match += mmr;
        if (mmr < min_mmr_match) {
            min_mmr_match = mmr;
        }
        if (mmr > max_mmr_match) {
            max_mmr_match = mmr;
        }
        auto wait_i = std::chrono::duration_cast<std::chrono::milliseconds>(now - queue[idx].queuedAt).count();
        if (wait_i < 0) {
            wait_i = 0;
        }
        total_wait_ms += wait_i;
        ++selected_count;
    }
    for (std::size_t idx : team_b) {
        selected[idx] = true;
        *outMatch.add_players() = queue[idx].player;

        int mmr = queue[idx].player.mmr();
        sum_mmr_match += mmr;
        if (mmr < min_mmr_match) {
            min_mmr_match = mmr;
        }
        if (mmr > max_mmr_match) {
            max_mmr_match = mmr;
        }
        auto wait_i = std::chrono::duration_cast<std::chrono::milliseconds>(now - queue[idx].queuedAt).count();
        if (wait_i < 0) {
            wait_i = 0;
        }
        total_wait_ms += wait_i;
        ++selected_count;
    }

    if (metrics) {
        if (selected_count > 0) {
            metrics->average_mmr = static_cast<double>(sum_mmr_match) / static_cast<double>(selected_count);
            metrics->min_mmr = min_mmr_match;
            metrics->max_mmr = max_mmr_match;
            metrics->average_wait_ms = static_cast<double>(total_wait_ms) / static_cast<double>(selected_count);
        } else {
            metrics->average_mmr = 0.0;
            metrics->min_mmr = 0;
            metrics->max_mmr = 0;
            metrics->average_wait_ms = 0.0;
        }
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
