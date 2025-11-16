#pragma once

#include <string>

struct EngineConfig {
    int tick_interval_ms = 100;
    std::string matches_path = "matches.jsonl";

    static EngineConfig LoadFromFile(const std::string& path);
};

