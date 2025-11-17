#pragma once

#include <string>

struct EngineConfig {
    int tick_interval_ms = 100;
    std::string matches_path = "matches.jsonl";

    int max_ping_ms = 80;
    int ping_relax_per_second = 10;
    int max_ping_ms_cap = 200;

    int min_wait_before_match_ms = 30000;
    int max_allowed_mmr_diff = 100;

    int base_mmr_window = 200;
    int mmr_relax_per_second = 50;
    int max_mmr_window = 1000;

    int mmr_diff_relax_per_second = 10;
    int max_relaxed_mmr_diff = 300;

    int cross_region_step_ms = 60000;
    int good_region_ping_ms = 100;

    static EngineConfig LoadFromFile(const std::string& path);
};

