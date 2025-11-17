#include <deque>

#include <gtest/gtest.h>

#include "Engine/MatchBuilder.h"
#include "Engine/PlayerEntry.h"
#include "Engine/EngineConfig.h"

using matchmaking::Player;
using matchmaking::Match;

namespace {

EngineConfig DefaultTestConfig() {
    EngineConfig cfg;
    cfg.tick_interval_ms = 100;
    cfg.matches_path = "matches.jsonl";
    cfg.max_ping_ms = 80;
    cfg.ping_relax_per_second = 10;
    cfg.max_ping_ms_cap = 200;
    return cfg;
}

}  // namespace

TEST(MatchBuilderTests, ReturnsFalseWhenQueueHasFewerThanTenPlayers) {
    std::deque<PlayerEntry> queue;
    Match match;
    EngineConfig config = DefaultTestConfig();

    Player p;
    p.set_id("p1");
    p.set_mmr(1000);
    p.set_ping(50);
    p.set_region("NA");

    queue.emplace_back(p);

    bool built = MatchBuilder::BuildMatch(queue, match, config);

    EXPECT_FALSE(built);
    EXPECT_TRUE(queue.size() == 1);
}

TEST(MatchBuilderTests, BuildsMatchWhenQueueHasAtLeastTenPlayers) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(1000 + i);
        p.set_ping(40 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_TRUE(queue.empty());
}

TEST(MatchBuilderTests, NotEnoughPlayersInsideMmrWindowDoesNotBuildMatch) {
    std::deque<PlayerEntry> queue;

    Player seed;
    seed.set_id("seed");
    seed.set_mmr(1000);
    seed.set_ping(50);
    seed.set_region("NA");
    queue.emplace_back(seed);

    for (int i = 0; i < 4; ++i) {
        Player p;
        p.set_id("near" + std::to_string(i));
        p.set_mmr(1000 + i * 10);
        p.set_ping(40 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    for (int i = 0; i < 5; ++i) {
        Player p;
        p.set_id("far" + std::to_string(i));
        p.set_mmr(2000 + i * 10);
        p.set_ping(80 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    EngineConfig config = DefaultTestConfig();

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    EXPECT_FALSE(built);
    EXPECT_EQ(queue.size(), 10u);
    EXPECT_EQ(queue.front().player.id(), "seed");
}

TEST(MatchBuilderTests, MatchUsesOnlyPlayersInsideMmrWindow) {
    std::deque<PlayerEntry> queue;

    Player seed;
    seed.set_id("seed");
    seed.set_mmr(1000);
    seed.set_ping(50);
    seed.set_region("NA");
    queue.emplace_back(seed);

    for (int i = 0; i < 9; ++i) {
        Player p;
        p.set_id("near" + std::to_string(i));
        p.set_mmr(1000 + i * 10);
        p.set_ping(40 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    for (int i = 0; i < 3; ++i) {
        Player p;
        p.set_id("far" + std::to_string(i));
        p.set_mmr(2000 + i * 10);
        p.set_ping(80 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    EngineConfig config = DefaultTestConfig();

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_EQ(queue.size(), 3u);

    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(queue[i].player.id(), "far" + std::to_string(i));
    }
}

TEST(MatchBuilderTests, TeamsAreReasonablyBalancedByMmr) {
    std::deque<PlayerEntry> queue;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(900 + i * 10);
        p.set_ping(40 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    EngineConfig config = DefaultTestConfig();

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    ASSERT_TRUE(built);
    ASSERT_EQ(match.players_size(), 10);

    int sum_a = 0;
    int sum_b = 0;
    for (int i = 0; i < 5; ++i) {
        sum_a += match.players(i).mmr();
    }
    for (int i = 5; i < 10; ++i) {
        sum_b += match.players(i).mmr();
    }

    int avg_a = sum_a / 5;
    int avg_b = sum_b / 5;
    int diff = avg_a - avg_b;
    if (diff < 0) diff = -diff;

    EXPECT_LT(diff, 300);
}

TEST(MatchBuilderTests, HighPingPlayersAreExcludedWhenUnderPingLimit) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.max_ping_ms = 80;
    config.ping_relax_per_second = 0;
    config.max_ping_ms_cap = 80;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("low" + std::to_string(i));
        p.set_mmr(1000);
        p.set_ping(50);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    for (int i = 0; i < 5; ++i) {
        Player p;
        p.set_id("high" + std::to_string(i));
        p.set_mmr(1000);
        p.set_ping(150);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    ASSERT_TRUE(built);
    ASSERT_EQ(match.players_size(), 10);
    EXPECT_EQ(queue.size(), 5u);

    for (int i = 0; i < match.players_size(); ++i) {
        EXPECT_LT(match.players(i).ping(), 100);
    }

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(queue[i].player.id(), "high" + std::to_string(i));
    }
}

TEST(MatchBuilderTests, NotEnoughLowPingPlayersNoMatch) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.max_ping_ms = 80;
    config.ping_relax_per_second = 0;
    config.max_ping_ms_cap = 80;

    for (int i = 0; i < 5; ++i) {
        Player p;
        p.set_id("low" + std::to_string(i));
        p.set_mmr(1000);
        p.set_ping(50);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("high" + std::to_string(i));
        p.set_mmr(1000);
        p.set_ping(150);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config);

    EXPECT_FALSE(built);
    EXPECT_EQ(queue.size(), 15u);
}
