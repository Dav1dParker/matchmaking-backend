# Matchmaking Simulation Backend

[![CI](https://github.com/dav1dparker/matchmaking-backend/actions/workflows/ci.yml/badge.svg)](https://github.com/dav1dparker/matchmaking-backend/actions/workflows/ci.yml)

This repository contains a small C++20 matchmaking system intended as a showcase project. The project simulates players joining a queue and being matched into balanced 5v5 games, using a gRPC backend and a separate simulation client.

## Overview

The system is split into two C++20 executables:

- `matchmaker_server` – gRPC backend that owns the matchmaking engine.
- `match_simulator` – gRPC client that generates synthetic players and enqueues them.

There is no real game; the focus is on backend logic, concurrency, and basic infrastructure (CI, Docker, CD).

## Matchmaking Rules

- Games are always 5 vs 5 (10 players per match).
- Supported regions: `NA`, `EU`, `ASIA`.
- Players have:
  - `id`
  - `mmr` (Elo-like rating)
  - `region` (home region, e.g., `NA`, `EU`, `ASIA`)
  - per-region pings: `ping_na`, `ping_eu`, `ping_asia` (ping to each region)
- Current constraints implemented:
  - Per-region queues (matches are built per region).
  - MMR window around the oldest player in the queue:
    - Starts at a base window around the seed player.
    - Grows over time based on the player’s time in queue, capped at a maximum window.
  - Players outside the current MMR window are not used for that match and remain in the queue.
  - Ping constraints that relax over time, capped by configuration.
  - Cross-region matching as a last resort, based on a configured step time.
  - Once 10 eligible players are found, they are split into two 5-player teams to keep team average MMR reasonably close.
- Metrics:
  - Match creation logs include average MMR, MMR spread, and average wait time for the players in the match.

The server records basic match statistics to an append-only `matches.jsonl` file for later inspection and analysis. The path and tick interval are configured via `config/server_config.json`.

## Architecture

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
  - Generates synthetic players with random `mmr`, home region, and per-region pings.
  - Sends `Enqueue` requests over gRPC to the server.
  - Uses `StreamMatches` to open a stream per player and print matches that contain that player.
  - Players play exactly one match and then disappear (no requeue yet).

- **Unit tests**
  - A small GoogleTest suite currently covers `MatchBuilder` behavior:
    - Minimum player count.
    - MMR window filtering.
    - Team balancing for 5v5 matches.

## Configuration

- `config/server_config.json`
  - Core fields include:
    - `tick_interval_ms`: engine tick interval in milliseconds.
    - `matches_path`: path to the JSONL file for match persistence.
    - `max_ping_ms`, `ping_relax_per_second`, `max_ping_ms_cap`: ping constraints and relaxation.
    - `min_wait_before_match_ms`, `max_allowed_mmr_diff`: initial MMR-diff constraints.
    - `base_mmr_window`, `mmr_relax_per_second`, `max_mmr_window`: MMR window behavior over time.
    - `mmr_diff_relax_per_second`, `max_relaxed_mmr_diff`: relaxed MMR-diff behavior.
    - `cross_region_step_ms`: step size for gradually allowing cross-region matches.
    - `good_region_ping_ms`: threshold that defines a “good” region ping.

- `config/sim_config.json`
  - `target_address`: gRPC address of the matchmaker server.
  - `total_players`: how many synthetic players the simulator enqueues.
  - `delay_ms_between_players`: delay between enqueue operations.
  - The `SIM_TARGET_ADDRESS` environment variable can override `target_address` (useful for Docker).

If a config file or key is missing, defaults are used.

## Interactive CLI (server and simulator)

Both executables start via a small menu when run in an interactive terminal:

- `matchmaker_server`:
  - Shows a menu:
    - `1) Run server`
    - `2) Change options`
    - `3) Reset options to defaults`
    - `4) Exit`
  - “Change options” lets you edit the values that are stored in `config/server_config.json`. Changes are saved before running.
  - “Reset options” restores defaults (the struct’s default values) and writes them back to `config/server_config.json`.

- `match_simulator`:
  - Shows a similar menu:
    - `1) Run simulator`
    - `2) Change options`
    - `3) Reset options to defaults`
    - `4) Exit`
  - “Change options” edits `config/sim_config.json` (target address, total players, and delay).

When there is no interactive input (for example, when running inside Docker without a TTY), both binaries detect EOF on stdin and automatically run once with the current configuration, rather than showing the menu in a loop.

Typical local usage (from the project root after a build):

```bash
./build/matchmaker_server
./build/match_simulator
```

## Docker & GHCR

Prebuilt images are published to GitHub Container Registry (GHCR) on pushes to `main`:

- Server: `ghcr.io/dav1dparker/matchmaking-server:latest`
- Simulator: `ghcr.io/dav1dparker/matchmaking-simulator:latest`

### Running server and simulator with Docker

Run the server (interactive menu, port 50051 exposed):

```bash
docker run -it --rm -p 50051:50051 ghcr.io/dav1dparker/matchmaking-server:latest
```

Run the simulator in another terminal:

```bash
docker run -it --rm ghcr.io/dav1dparker/matchmaking-simulator:latest
```

To run server and simulator so they can see each other inside Docker, use a user-defined network and point the simulator at the server container name:

```bash
docker network create matchmaking

docker run -it --rm \
  --name match-server \
  --network matchmaking \
  -p 50051:50051 \
  ghcr.io/dav1dparker/matchmaking-server:latest

docker run -it --rm \
  --network matchmaking \
  -e SIM_TARGET_ADDRESS=match-server:50051 \
  ghcr.io/dav1dparker/matchmaking-simulator:latest
```

To override configuration in the container with local files, mount your `config` directory into `/app/config`:

```bash
docker run -it --rm \
  -v /path/to/config:/app/config:ro \
  ghcr.io/dav1dparker/matchmaking-server:latest
```

## CI status and future work

- **CI (GitHub Actions)** is configured and running:
  - Uses vcpkg to install gRPC, Protobuf, and GoogleTest.
  - Configures and builds both executables with CMake/Ninja.
  - Runs the `matchmaking_tests` GoogleTest suite for `MatchBuilder`.
  - Uploads the CMake `build/` directory as an artifact.
  - On pushes to `main`, builds lightweight runtime Docker images from the prebuilt binaries and pushes them to GHCR.

- **Planned next steps**
  - Add a small metrics endpoint to expose queue sizes per region, match counts, and averaged match statistics while the server runs.
  - Extend simulator configuration to support more realistic arrival patterns and per-region controls.

This project is intentionally compact but structured like a real service, to demonstrate backend design, concurrency, gRPC usage, CI, and Docker/CD practices.
