#include "MatchPersistence.h"

#include <fstream>

MatchPersistence::MatchPersistence(const std::string& path)
    : path_(path) {}

void MatchPersistence::Append(const matchmaking::Match& match) {
    std::ofstream out(path_, std::ios::app);
    if (!out.is_open()) {
        return;
    }

    out << "{";
    out << "\"match_id\":\"" << match.match_id() << "\"";
    out << ",\"players\":[";
    for (int i = 0; i < match.players_size(); ++i) {
        const auto& p = match.players(i);
        if (i > 0) {
            out << ",";
        }
        out << "{";
        out << "\"id\":\"" << p.id() << "\"";
        out << ",\"mmr\":" << p.mmr();
        out << ",\"ping\":" << p.ping();
        out << ",\"region\":\"" << p.region() << "\"";
        out << "}";
    }
    out << "]";
    out << "}\n";
}

