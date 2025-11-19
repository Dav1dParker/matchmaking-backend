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

    std::sort(regions, regions + 3,
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
    if (queue.size() < 10) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    const std::size_t n = queue.size();

    // Precompute wait times for all players once.
    std::vector<long long> wait_ms(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
        auto w = std::chrono::duration_cast<std::chrono::milliseconds>(now - queue[i].queuedAt).count();
        if (w < 0) {
            w = 0;
        }
        wait_ms[i] = w;
    }

    struct SeedChoice {
        bool valid = false;
        std::size_t seed_index = 0;
        std::vector<std::size_t> selected_indices;
        long long seed_wait_ms = 0;
        double avg_wait_ms = 0.0;
        int spread = 0;
    };

    SeedChoice best;

    // Consider every player as a potential seed for this region.
    for (std::size_t seed_index = 0; seed_index < n; ++seed_index) {
        long long waited_ms = wait_ms[seed_index];

        if (!IsRegionAllowedForPlayer(queue[seed_index], region, waited_ms, config)) {
            continue;
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

        const int seed_mmr = queue[seed_index].player.mmr();
        const int min_mmr = seed_mmr - window;
        const int max_mmr = seed_mmr + window;

        int ping_window = config.max_ping_ms +
                          config.ping_relax_per_second * relax_seconds;
        if (ping_window > config.max_ping_ms_cap) {
            ping_window = config.max_ping_ms_cap;
        }

        std::vector<std::size_t> eligible_indices;
        eligible_indices.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            long long wait_i = wait_ms[i];
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

        if (eligible_indices.size() < 10) {
            continue;
        }

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

        int best_start_for_seed = -1;
        int best_spread_for_seed = std::numeric_limits<int>::max();
        for (std::size_t i = 0; i + 9 < all_candidates.size(); ++i) {
            int spread = all_candidates[i + 9].mmr - all_candidates[i].mmr;
            if (spread < best_spread_for_seed) {
                best_spread_for_seed = spread;
                best_start_for_seed = static_cast<int>(i);
            }
        }

        if (best_start_for_seed < 0) {
            continue;
        }

        int allowed_spread = config.max_allowed_mmr_diff;
        if (waited_ms > config.min_wait_before_match_ms) {
            allowed_spread += config.mmr_diff_relax_per_second * relax_seconds;
            if (allowed_spread > config.max_relaxed_mmr_diff) {
                allowed_spread = config.max_relaxed_mmr_diff;
            }
        }

        if (best_spread_for_seed > allowed_spread) {
            continue;
        }

        std::vector<std::size_t> selected_indices;
        selected_indices.reserve(10);
        long long sum_wait_ms = 0;

        for (int j = 0; j < 10; ++j) {
            std::size_t idx = all_candidates[best_start_for_seed + j].index;
            selected_indices.push_back(idx);
            sum_wait_ms += wait_ms[idx];
        }

        double avg_wait_ms = static_cast<double>(sum_wait_ms) / 10.0;

        bool take = false;
        if (!best.valid) {
            take = true;
        } else if (avg_wait_ms > best.avg_wait_ms) {
            take = true;
        } else if (avg_wait_ms == best.avg_wait_ms &&
                   best_spread_for_seed < best.spread) {
            take = true;
        }

        if (take) {
            best.valid = true;
            best.seed_index = seed_index;
            best.selected_indices = std::move(selected_indices);
            best.seed_wait_ms = waited_ms;
            best.avg_wait_ms = avg_wait_ms;
            best.spread = best_spread_for_seed;
        }
    }

    if (!best.valid) {
        long long emergency_ms = config.emergency_match_wait_ms;
        if (region == "NA" && emergency_ms > 0) {
            std::vector<std::size_t> long_wait_indices;
            long_wait_indices.reserve(n);
            for (std::size_t i = 0; i < n; ++i) {
                if (wait_ms[i] >= emergency_ms) {
                    long_wait_indices.push_back(i);
                }
            }
            if (long_wait_indices.size() >= 10) {
                best.valid = true;
                best.seed_index = long_wait_indices[0];
                best.selected_indices.assign(long_wait_indices.begin(),
                                             long_wait_indices.begin() + 10);
                long long sum_wait = 0;
                for (std::size_t idx : best.selected_indices) {
                    sum_wait += wait_ms[idx];
                }
                best.avg_wait_ms = static_cast<double>(sum_wait) / 10.0;
                best.spread = 0;
            }
        }
        if (!best.valid) {
            return false;
        }
    }

    outMatch.set_match_id("match_" + std::to_string(rand()));

    struct TeamCandidate {
        std::size_t index;
        int mmr;
    };

    std::vector<TeamCandidate> candidates;
    candidates.reserve(10);
    for (std::size_t idx : best.selected_indices) {
        candidates.push_back(TeamCandidate{idx, queue[idx].player.mmr()});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const TeamCandidate& a, const TeamCandidate& b) {
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

    std::vector<bool> selected_flags(n, false);

    long long total_wait_ms = 0;
    long long sum_mmr_match = 0;
    int min_mmr_match = std::numeric_limits<int>::max();
    int max_mmr_match = std::numeric_limits<int>::min();
    int selected_count = 0;

    auto add_player_to_match = [&](std::size_t idx) {
        selected_flags[idx] = true;
        *outMatch.add_players() = queue[idx].player;

        int mmr = queue[idx].player.mmr();
        sum_mmr_match += mmr;
        if (mmr < min_mmr_match) {
            min_mmr_match = mmr;
        }
        if (mmr > max_mmr_match) {
            max_mmr_match = mmr;
        }
        long long w = wait_ms[idx];
        if (w < 0) {
            w = 0;
        }
        total_wait_ms += w;
        ++selected_count;
    };

    for (std::size_t idx : team_a) {
        add_player_to_match(idx);
    }
    for (std::size_t idx : team_b) {
        add_player_to_match(idx);
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

    for (std::size_t i = 0; i < n; ++i) {
        if (!selected_flags[i]) {
            remaining.push_back(queue[i]);
        }
    }

    queue.swap(remaining);

    return true;
}
