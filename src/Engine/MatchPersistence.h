#pragma once

#include <string>

#include "matchmaker.pb.h"

class MatchPersistence {
public:
    explicit MatchPersistence(const std::string& path);

    void Append(const matchmaking::Match& match);

private:
    std::string path_;
};

