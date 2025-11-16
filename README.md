# Matchmaking Simulation Backend

This repository contains a small C++20 matchmaking system intended as a showcase project. The project simulates players joining a queue and being matched into balanced 5v5 games, using a gRPC backend and a separate simulation client.

## Overview

The system is split into two C++20 executables:

- `matchmaker_server` – gRPC backend that owns the matchmaking engine.
- `match_simulator` – gRPC client that generates synthetic players and enqueues them.

There is no real game; the focus is on backend logic, concurrency, and basic infrastructure (CI, later Docker/CD).

## Matchmaking Rules (current)

- Games are always 5 vs 5 (10 players per match).
- Supported regions: `NA`, `EU`, `ASIA`.
- Players have:
  - `id`
  - `mmr` (Elo-like rating)
  - `ping`
  - `region`
- Current constraints implemented:
  - Per-region queues (matches are built per region; cross-region is not implemented yet).
  - MMR window around the oldest player in the queue:
    - Starts at ±200.
    - Grows over time based on the player’s time in queue, capped at a maximum window.
  - Players outside the current MMR window are not used for that match and remain in the queue.
  - Once 10 eligible players are found, they are split into two 5-player teams to keep team average MMR reasonably close.
- Planned (not implemented yet):
  - Explicit ping constraints and time-based ping relaxation.
  - Cross-region matching as a last resort once a player has waited beyond a threshold.

The server records basic match statistics to an append-only `matches.jsonl` file for later inspection and analysis. The path and tick interval are configured via `config/server_config.json`.

## Architecture (current)

- **Protocol Buffers / gRPC**
  - `proto/matchmaker.proto` defines:
    - `Enqueue(Player) -> EnqueueResponse`
    - `Cancel(PlayerID) -> CancelResponse`
    - `StreamMatches(PlayerID) -> stream Match`
  - Code is generated into the `generated/` folder.

- **Server (`matchmaker_server`)**
  - Listens on `0.0.0.0:50051` by default.
  - Implements the `Matchmaker` service using a matchmaking engine that:
    - Maintains per-region queues of players (`PlayerEntry` with time-in-queue).
    - Runs a background tick loop (interval from `config/server_config.json`) and uses `MatchBuilder` to build matches.
    - Forms 5v5 games using MMR window filtering and simple team balancing.
  - Persists match stats to `matches.jsonl` via `MatchPersistence`.

- **Simulator (`match_simulator`)**
  - Reads configuration from `config/sim_config.json`:
    - `target_address`
    - `total_players`
    - `delay_ms_between_players`
  - Generates synthetic players with random `mmr`, `ping`, and region.
  - Sends `Enqueue` requests over gRPC to the server.
  - Players play exactly one match and then disappear (no requeue yet).

- **Unit tests**
  - A small GoogleTest suite currently covers `MatchBuilder` behavior:
    - Minimum player count.
    - MMR window filtering.
    - Team balancing for 5v5 matches.

## Configuration

- `config/server_config.json`
  - `tick_interval_ms`: engine tick interval in milliseconds.
  - `matches_path`: path to the JSONL file for match persistence.

- `config/sim_config.json`
  - `target_address`: gRPC address of the matchmaker server.
  - `total_players`: how many synthetic players the simulator enqueues.
  - `delay_ms_between_players`: delay between enqueue operations.

If a config file or key is missing, defaults are used.

## CI status and future work

- **CI (GitHub Actions)** is configured and running:
  - Uses vcpkg to install gRPC, Protobuf, and GoogleTest.
  - Configures and builds both executables with CMake/Ninja.
  - Runs the `matchmaking_tests` GoogleTest suite for `MatchBuilder`.

- **Planned next steps**
  - Add ping-based constraints and cross-region behavior to the engine.
  - Add a streaming view of matches in the simulator using `StreamMatches`.
  - Introduce Dockerfiles for both binaries and extend CI into CD to build images.

This project is intentionally compact but structured like a real service, to demonstrate backend design, concurrency, gRPC usage, CI, and eventually Docker/CD practices.

