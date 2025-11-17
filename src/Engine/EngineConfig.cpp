#include "EngineConfig.h"

#include <fstream>
#include <string>

namespace {

bool ExtractInt(const std::string& content, const std::string& key, int& out) {
    std::size_t key_pos = content.find("\"" + key + "\"");
    if (key_pos == std::string::npos) {
        return false;
    }
    std::size_t colon_pos = content.find(':', key_pos);
    if (colon_pos == std::string::npos) {
        return false;
    }
    std::size_t start = colon_pos + 1;
    while (start < content.size() && (content[start] == ' ' || content[start] == '\t' || content[start] == '\n' || content[start] == '\r')) {
        ++start;
    }
    std::size_t end = start;
    while (end < content.size() && (content[end] == '-' || (content[end] >= '0' && content[end] <= '9'))) {
        ++end;
    }
    if (start == end) {
        return false;
    }
    try {
        out = std::stoi(content.substr(start, end - start));
    } catch (...) {
        return false;
    }
    return true;
}

bool ExtractString(const std::string& content, const std::string& key, std::string& out) {
    std::size_t key_pos = content.find("\"" + key + "\"");
    if (key_pos == std::string::npos) {
        return false;
    }
    std::size_t colon_pos = content.find(':', key_pos);
    if (colon_pos == std::string::npos) {
        return false;
    }
    std::size_t start = content.find('"', colon_pos + 1);
    if (start == std::string::npos) {
        return false;
    }
    ++start;
    std::size_t end = content.find('"', start);
    if (end == std::string::npos) {
        return false;
    }
    out = content.substr(start, end - start);
    return true;
}

}  // namespace

EngineConfig EngineConfig::LoadFromFile(const std::string& path) {
    EngineConfig config;

    std::ifstream in(path);
    if (!in.is_open()) {
        return config;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    int tick_value = config.tick_interval_ms;
    if (ExtractInt(content, "tick_interval_ms", tick_value)) {
        config.tick_interval_ms = tick_value;
    }

    std::string matches_value = config.matches_path;
    if (ExtractString(content, "matches_path", matches_value)) {
        config.matches_path = matches_value;
    }

    int max_ping_value = config.max_ping_ms;
    if (ExtractInt(content, "max_ping_ms", max_ping_value)) {
        config.max_ping_ms = max_ping_value;
    }

    int relax_value = config.ping_relax_per_second;
    if (ExtractInt(content, "ping_relax_per_second", relax_value)) {
        config.ping_relax_per_second = relax_value;
    }

    int ping_cap_value = config.max_ping_ms_cap;
    if (ExtractInt(content, "max_ping_ms_cap", ping_cap_value)) {
        config.max_ping_ms_cap = ping_cap_value;
    }

    return config;
}

