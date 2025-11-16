#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "simulator/SimulatorClient.h"
#include "matchmaker.pb.h"

static matchmaking::Player MakeDummyPlayer(int index) {
    static std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> mmr_dist(800, 2400);
    std::uniform_int_distribution<int> ping_dist(20, 150);

    const std::vector<std::string> regions = {"NA", "EU", "ASIA"};
    std::uniform_int_distribution<std::size_t> region_dist(0, regions.size() - 1);

    matchmaking::Player player;
    player.set_id("player_" + std::to_string(index));
    player.set_mmr(mmr_dist(rng));
    player.set_ping(ping_dist(rng));
    player.set_region(regions[region_dist(rng)]);

    return player;
}

int main() {
    const std::string target_address = "localhost:50051";
    constexpr int total_players = 100;
    constexpr int delay_ms_between_players = 10;

    std::cout << "Starting match_simulator, target=" << target_address
              << ", total_players=" << total_players << std::endl;

    SimulatorClient client(target_address);

    for (int i = 0; i < total_players; ++i) {
        auto player = MakeDummyPlayer(i);

        bool ok = client.Enqueue(player);
        if (!ok) {
            std::cerr << "Failed to enqueue player " << player.id() << "\n";
        } else {
            std::cout << "Enqueued player " << player.id()
                      << " (mmr=" << player.mmr()
                      << ", ping=" << player.ping()
                      << ", region=" << player.region() << ")\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_between_players));
    }

    std::cout << "Simulation finished.\n";
    return 0;
}

