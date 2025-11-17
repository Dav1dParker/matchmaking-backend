#pragma once

#include <string>

struct EngineConfig {
    int tick_interval_ms = 100;
    std::string matches_path = "matches.jsonl";

    int max_ping_ms = 80;
    int ping_relax_per_second = 10;
    int max_ping_ms_cap = 200;

    int min_wait_before_match_ms = 30000;
    int max_allowed_mmr_diff = 300;

    static EngineConfig LoadFromFile(const std::string& path);
};

