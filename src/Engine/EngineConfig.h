#pragma once

#include <string>

struct EngineConfig {
    int tick_interval_ms = 100;
    std::string matches_path = "matches.jsonl";

    int max_ping_ms = 80;
    int ping_relax_per_second = 10;
    int max_ping_ms_cap = 200;

    static EngineConfig LoadFromFile(const std::string& path);
};

