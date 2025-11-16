#include "simulator/SimConfig.h"

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

SimConfig SimConfig::LoadFromFile(const std::string& path) {
    SimConfig config;

    std::ifstream in(path);
    if (!in.is_open()) {
        return config;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    int total_value = config.total_players;
    if (ExtractInt(content, "total_players", total_value)) {
        config.total_players = total_value;
    }

    int delay_value = config.delay_ms_between_players;
    if (ExtractInt(content, "delay_ms_between_players", delay_value)) {
        config.delay_ms_between_players = delay_value;
    }

    std::string address_value = config.target_address;
    if (ExtractString(content, "target_address", address_value)) {
        config.target_address = address_value;
    }

    return config;
}

