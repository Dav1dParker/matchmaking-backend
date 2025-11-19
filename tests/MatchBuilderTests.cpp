#include <deque>
#include <chrono>

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
    cfg.min_wait_before_match_ms = 0;
    cfg.max_allowed_mmr_diff = 1000;
    cfg.base_mmr_window = 200;
    cfg.mmr_relax_per_second = 50;
    cfg.max_mmr_window = 1000;
    cfg.mmr_diff_relax_per_second = 0;
    cfg.max_relaxed_mmr_diff = cfg.max_allowed_mmr_diff;
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

    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

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
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

    EXPECT_FALSE(built);
    EXPECT_EQ(queue.size(), 15u);
}

TEST(MatchBuilderTests, PingRelaxationOverTimeAllowsHighPingPlayers) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.max_ping_ms = 80;
    config.ping_relax_per_second = 10;
    config.max_ping_ms_cap = 200;

    Player seed;
    seed.set_id("seed");
    seed.set_mmr(1500);
    seed.set_ping(150);
    seed.set_region("NA");
    queue.emplace_back(seed);

    for (int i = 0; i < 9; ++i) {
        Player p;
        p.set_id("high" + std::to_string(i));
        p.set_mmr(1500);
        p.set_ping(150);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    queue.front().queuedAt -= std::chrono::seconds(10);

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

    ASSERT_TRUE(built);
    ASSERT_EQ(match.players_size(), 10);
    EXPECT_TRUE(queue.empty());

    for (int i = 0; i < match.players_size(); ++i) {
        EXPECT_GE(match.players(i).ping(), 100);
    }
}

TEST(MatchBuilderTests, PingWindowIsCappedByMaxPingCap) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.max_ping_ms = 50;
    config.ping_relax_per_second = 50;
    config.max_ping_ms_cap = 120;

    Player seed;
    seed.set_id("seed");
    seed.set_mmr(1500);
    seed.set_ping(100);
    seed.set_region("NA");
    queue.emplace_back(seed);

    for (int i = 0; i < 4; ++i) {
        Player p;
        p.set_id("ok" + std::to_string(i));
        p.set_mmr(1500);
        p.set_ping(100);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("too_high" + std::to_string(i));
        p.set_mmr(1500);
        p.set_ping(200);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    queue.front().queuedAt -= std::chrono::seconds(10);

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

    EXPECT_FALSE(built);
    EXPECT_EQ(queue.size(), 15u);
}

TEST(MatchBuilderTests, RejectsUnbalancedMatchBeforeMinWait) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.min_wait_before_match_ms = 30000;
    config.max_allowed_mmr_diff = 0;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(900 + i * 10);
        p.set_ping(50);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

    EXPECT_FALSE(built);
    EXPECT_EQ(queue.size(), 10u);
}

TEST(MatchBuilderTests, AcceptsUnbalancedMatchAfterMinWait) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.min_wait_before_match_ms = 30000;
    config.max_allowed_mmr_diff = 1000;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(900 + i * 10);
        p.set_ping(50);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    queue.front().queuedAt -= std::chrono::milliseconds(31000);

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA");

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_TRUE(queue.empty());
}

TEST(MatchBuilderTests, GoodRegionPingAllowsCrossRegionWithoutWait) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.good_region_ping_ms = 60;
    config.cross_region_step_ms = 60000;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(1500);
        p.set_ping_na(20);
        p.set_ping_eu(40);
        p.set_ping_asia(200);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "EU");

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_TRUE(queue.empty());
}

TEST(MatchBuilderTests, CrossRegionRequiresWaitBeforeAllow) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();
    config.good_region_ping_ms = 50;
    config.cross_region_step_ms = 20000;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(1500);
        p.set_ping_na(20);
        p.set_ping_eu(80);
        p.set_ping_asia(200);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built_without_wait = MatchBuilder::BuildMatch(queue, match, config, "EU");

    EXPECT_FALSE(built_without_wait);
    EXPECT_EQ(queue.size(), 10u);

    for (auto& entry : queue) {
        entry.queuedAt -= std::chrono::seconds(21);
    }

    Match match_after_wait;
    bool built_after_wait = MatchBuilder::BuildMatch(queue, match_after_wait, config, "EU");

    EXPECT_TRUE(built_after_wait);
    EXPECT_EQ(match_after_wait.players_size(), 10);
    EXPECT_TRUE(queue.empty());
}

TEST(MatchBuilderTests, MetricsAreComputedForBuiltMatch) {
    std::deque<PlayerEntry> queue;

    EngineConfig config = DefaultTestConfig();

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(1000 + i * 10);
        p.set_ping(40);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    MatchMetrics metrics;
    bool built = MatchBuilder::BuildMatch(queue, match, config, "NA", &metrics);

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_TRUE(queue.empty());

    EXPECT_DOUBLE_EQ(metrics.average_mmr, 1045.0);
    EXPECT_EQ(metrics.min_mmr, 1000);
    EXPECT_EQ(metrics.max_mmr, 1090);
    EXPECT_GE(metrics.average_wait_ms, 0.0);
}
