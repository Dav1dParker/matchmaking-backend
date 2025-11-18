#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "simulator/SimulatorClient.h"
#include "simulator/SimConfig.h"
#include "matchmaker.pb.h"

namespace {

matchmaking::Player MakeDummyPlayer(int index) {
    static std::mt19937 rng(std::random_device{}());

    std::uniform_int_distribution<int> mmr_dist(800, 2400);
    std::uniform_int_distribution<int> base_ping_dist(20, 60);
    std::uniform_int_distribution<int> extra_ping_dist(60, 140);

    matchmaking::Player player;
    player.set_id("player_" + std::to_string(index));
    player.set_mmr(mmr_dist(rng));

    const std::vector<std::string> regions = {"NA", "EU", "ASIA"};
    std::uniform_int_distribution<std::size_t> home_dist(0, regions.size() - 1);
    std::size_t home_index = home_dist(rng);

    int base_ping = base_ping_dist(rng);

    int ping_na = base_ping + (home_index == 0 ? 0 : extra_ping_dist(rng));
    int ping_eu = base_ping + (home_index == 1 ? 0 : extra_ping_dist(rng));
    int ping_asia = base_ping + (home_index == 2 ? 0 : extra_ping_dist(rng));

    player.set_ping_na(ping_na);
    player.set_ping_eu(ping_eu);
    player.set_ping_asia(ping_asia);

    std::string home_region = regions[home_index];
    int best_ping = base_ping;

    player.set_ping(best_ping);
    player.set_region(home_region);

    return player;
}

int RunSimulator(const SimConfig& config) {
    std::cout << "Starting match_simulator, target=" << config.target_address
              << ", total_players=" << config.total_players << std::endl;

    SimulatorClient client(config.target_address);
    std::vector<std::thread> stream_threads;
    stream_threads.reserve(config.total_players);

    for (int i = 0; i < config.total_players; ++i) {
        auto player = MakeDummyPlayer(i);

        std::string player_id = player.id();
        stream_threads.emplace_back([player_id, &client]() {
            client.StreamMatches(player_id);
        });

        bool ok = client.Enqueue(player);
        if (!ok) {
            std::cerr << "Failed to enqueue player " << player.id() << "\n";
        } else {
            std::cout << "Enqueued player " << player.id()
                      << " (mmr=" << player.mmr()
                      << ", ping_na=" << player.ping_na()
                      << ", ping_eu=" << player.ping_eu()
                      << ", ping_asia=" << player.ping_asia()
                      << ", home_region=" << player.region() << ")\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(config.delay_ms_between_players));
    }

    std::cout << "Simulation finished, waiting for match streams.\n";

    for (auto& t : stream_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}

void EditSimConfig(SimConfig& config) {
    for (;;) {
        std::cout << "\nCurrent simulator configuration:\n";
        std::cout << "1) target_address = " << config.target_address << "\n";
        std::cout << "2) total_players = " << config.total_players << "\n";
        std::cout << "3) delay_ms_between_players = " << config.delay_ms_between_players << "\n";
        std::cout << "4) Back\n";
        std::cout << "Select setting to change: ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (choice == 4) {
            break;
        }

        if (choice == 1) {
            std::cout << "Enter new target_address: ";
            std::cin >> config.target_address;
            continue;
        }

        std::cout << "Enter new value: ";
        int value = 0;
        if (!(std::cin >> value)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
            case 2:
                config.total_players = value;
                break;
            case 3:
                config.delay_ms_between_players = value;
                break;
            default:
                break;
        }
    }
}

}  // namespace

int main() {
    SimConfig config = SimConfig::LoadFromFile("config/sim_config.json");

    for (;;) {
        std::cout << "\nMatch simulator menu:\n";
        std::cout << "1) Run simulator\n";
        std::cout << "2) Change options\n";
        std::cout << "3) Reset options to defaults\n";
        std::cout << "4) Exit\n";
        std::cout << "Select option: ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1) {
            config.SaveToFile("config/sim_config.json");
            return RunSimulator(config);
        } else if (choice == 2) {
            EditSimConfig(config);
            config.SaveToFile("config/sim_config.json");
        } else if (choice == 3) {
            config = SimConfig();
            config.SaveToFile("config/sim_config.json");
            std::cout << "Options reset to defaults.\n";
        } else if (choice == 4) {
            break;
        }
    }

    return 0;
}
