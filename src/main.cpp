#include <iostream>
#include <limits>
#include <grpcpp/grpcpp.h>
#include "server.h"
#include "Engine/Engine.h"
#include "Engine/EngineConfig.h"

namespace {

int RunServer() {
    std::string server_address("0.0.0.0:50051");
    MatchmakerServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Matchmaker server running on " << server_address << std::endl;
    server->Wait();

    return 0;
}

void EditEngineConfig(EngineConfig& config) {
    for (;;) {
        std::cout << "\nCurrent server configuration:\n";
        std::cout << "1) tick_interval_ms = " << config.tick_interval_ms << "\n";
        std::cout << "2) matches_path = " << config.matches_path << "\n";
        std::cout << "3) max_ping_ms = " << config.max_ping_ms << "\n";
        std::cout << "4) ping_relax_per_second = " << config.ping_relax_per_second << "\n";
        std::cout << "5) max_ping_ms_cap = " << config.max_ping_ms_cap << "\n";
        std::cout << "6) min_wait_before_match_ms = " << config.min_wait_before_match_ms << "\n";
        std::cout << "7) max_allowed_mmr_diff = " << config.max_allowed_mmr_diff << "\n";
        std::cout << "8) base_mmr_window = " << config.base_mmr_window << "\n";
        std::cout << "9) mmr_relax_per_second = " << config.mmr_relax_per_second << "\n";
        std::cout << "10) max_mmr_window = " << config.max_mmr_window << "\n";
        std::cout << "11) mmr_diff_relax_per_second = " << config.mmr_diff_relax_per_second << "\n";
        std::cout << "12) max_relaxed_mmr_diff = " << config.max_relaxed_mmr_diff << "\n";
        std::cout << "13) cross_region_step_ms = " << config.cross_region_step_ms << "\n";
        std::cout << "14) good_region_ping_ms = " << config.good_region_ping_ms << "\n";
        std::cout << "15) Back\n";
        std::cout << "Select setting to change: ";

        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (choice == 15) {
            break;
        }

        if (choice == 2) {
            std::cout << "Enter new matches_path: ";
            std::cin >> config.matches_path;
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
            case 1:
                config.tick_interval_ms = value;
                break;
            case 3:
                config.max_ping_ms = value;
                break;
            case 4:
                config.ping_relax_per_second = value;
                break;
            case 5:
                config.max_ping_ms_cap = value;
                break;
            case 6:
                config.min_wait_before_match_ms = value;
                break;
            case 7:
                config.max_allowed_mmr_diff = value;
                break;
            case 8:
                config.base_mmr_window = value;
                break;
            case 9:
                config.mmr_relax_per_second = value;
                break;
            case 10:
                config.max_mmr_window = value;
                break;
            case 11:
                config.mmr_diff_relax_per_second = value;
                break;
            case 12:
                config.max_relaxed_mmr_diff = value;
                break;
            case 13:
                config.cross_region_step_ms = value;
                break;
            case 14:
                config.good_region_ping_ms = value;
                break;
            default:
                break;
        }
    }
}

}  // namespace

int main() {
    EngineConfig config = EngineConfig::LoadFromFile("config/server_config.json");

    for (;;) {
        std::cout << "\nMatchmaker server menu:\n";
        std::cout << "1) Run server\n";
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
            config.SaveToFile("config/server_config.json");
            return RunServer();
        } else if (choice == 2) {
            EditEngineConfig(config);
            config.SaveToFile("config/server_config.json");
        } else if (choice == 3) {
            config = EngineConfig();
            config.SaveToFile("config/server_config.json");
            std::cout << "Options reset to defaults.\n";
        } else if (choice == 4) {
            break;
        }
    }

    return 0;
}
