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

    int min_wait_value = config.min_wait_before_match_ms;
    if (ExtractInt(content, "min_wait_before_match_ms", min_wait_value)) {
        config.min_wait_before_match_ms = min_wait_value;
    }

    int max_diff_value = config.max_allowed_mmr_diff;
    if (ExtractInt(content, "max_allowed_mmr_diff", max_diff_value)) {
        config.max_allowed_mmr_diff = max_diff_value;
    }

    int base_window_value = config.base_mmr_window;
    if (ExtractInt(content, "base_mmr_window", base_window_value)) {
        config.base_mmr_window = base_window_value;
    }

    int relax_mmr_value = config.mmr_relax_per_second;
    if (ExtractInt(content, "mmr_relax_per_second", relax_mmr_value)) {
        config.mmr_relax_per_second = relax_mmr_value;
    }

    int max_window_value = config.max_mmr_window;
    if (ExtractInt(content, "max_mmr_window", max_window_value)) {
        config.max_mmr_window = max_window_value;
    }

    int diff_relax_value = config.mmr_diff_relax_per_second;
    if (ExtractInt(content, "mmr_diff_relax_per_second", diff_relax_value)) {
        config.mmr_diff_relax_per_second = diff_relax_value;
    }

    int max_relaxed_diff_value = config.max_relaxed_mmr_diff;
    if (ExtractInt(content, "max_relaxed_mmr_diff", max_relaxed_diff_value)) {
        config.max_relaxed_mmr_diff = max_relaxed_diff_value;
    }

    int cross_region_step_value = config.cross_region_step_ms;
    if (ExtractInt(content, "cross_region_step_ms", cross_region_step_value)) {
        config.cross_region_step_ms = cross_region_step_value;
    }

    int good_region_ping_value = config.good_region_ping_ms;
    if (ExtractInt(content, "good_region_ping_ms", good_region_ping_value)) {
        config.good_region_ping_ms = good_region_ping_value;
    }

    return config;
}

bool EngineConfig::SaveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << "{\n";
    out << "  \"tick_interval_ms\": " << tick_interval_ms << ",\n";
    out << "  \"matches_path\": \"" << matches_path << "\",\n";
    out << "  \"max_ping_ms\": " << max_ping_ms << ",\n";
    out << "  \"ping_relax_per_second\": " << ping_relax_per_second << ",\n";
    out << "  \"max_ping_ms_cap\": " << max_ping_ms_cap << ",\n";
    out << "  \"min_wait_before_match_ms\": " << min_wait_before_match_ms << ",\n";
    out << "  \"max_allowed_mmr_diff\": " << max_allowed_mmr_diff << ",\n";
    out << "  \"base_mmr_window\": " << base_mmr_window << ",\n";
    out << "  \"mmr_relax_per_second\": " << mmr_relax_per_second << ",\n";
    out << "  \"max_mmr_window\": " << max_mmr_window << ",\n";
    out << "  \"mmr_diff_relax_per_second\": " << mmr_diff_relax_per_second << ",\n";
    out << "  \"max_relaxed_mmr_diff\": " << max_relaxed_mmr_diff << ",\n";
    out << "  \"cross_region_step_ms\": " << cross_region_step_ms << ",\n";
    out << "  \"good_region_ping_ms\": " << good_region_ping_ms << "\n";
    out << "}\n";

    return true;
}

