#pragma once

#include <string>

struct SimConfig {
    int total_players = 100;
    int delay_ms_between_players = 10;
    std::string target_address = "localhost:50051";

    static SimConfig LoadFromFile(const std::string& path);
    bool SaveToFile(const std::string& path) const;
};

