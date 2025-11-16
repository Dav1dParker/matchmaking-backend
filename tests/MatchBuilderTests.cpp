#include <deque>

#include <gtest/gtest.h>

#include "Engine/MatchBuilder.h"
#include "Engine/PlayerEntry.h"

using matchmaking::Player;
using matchmaking::Match;

TEST(MatchBuilderTests, ReturnsFalseWhenQueueHasFewerThanTenPlayers) {
    std::deque<PlayerEntry> queue;
    Match match;

    Player p;
    p.set_id("p1");
    p.set_mmr(1000);
    p.set_ping(50);
    p.set_region("NA");

    queue.emplace_back(p);

    bool built = MatchBuilder::BuildMatch(queue, match);

    EXPECT_FALSE(built);
    EXPECT_TRUE(queue.size() == 1);
}

TEST(MatchBuilderTests, BuildsMatchWhenQueueHasAtLeastTenPlayers) {
    std::deque<PlayerEntry> queue;

    for (int i = 0; i < 10; ++i) {
        Player p;
        p.set_id("p" + std::to_string(i));
        p.set_mmr(1000 + i);
        p.set_ping(40 + i);
        p.set_region("NA");
        queue.emplace_back(p);
    }

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match);

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

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match);

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

    Match match;
    bool built = MatchBuilder::BuildMatch(queue, match);

    EXPECT_TRUE(built);
    EXPECT_EQ(match.players_size(), 10);
    EXPECT_EQ(queue.size(), 3u);

    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(queue[i].player.id(), "far" + std::to_string(i));
    }
}
