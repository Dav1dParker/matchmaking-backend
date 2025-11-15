# Matchmaking Simulation Backend

This repository contains a small C++20 matchmaking system intended as a showcase project (e.g., for a Riot Games internship application). The project simulates players joining a queue and being matched into balanced 5v5 games, using a gRPC backend and a separate simulation client.

## Overview

The system is split into two executables that run in separate Docker containers:

- `matchmaker_server` – gRPC backend that owns the matchmaking engine.
- `match_simulator` – gRPC client that generates synthetic players and enqueues them.

There is **no real game**; the focus is on backend logic, concurrency, and basic infrastructure (CI/CD, Docker).

## Matchmaking Rules (v1)

- Games are always **5 vs 5** (10 players per match).
- Supported regions: **NA**, **EU**, **ASIA**.
- Players have:
  - `id`
  - `mmr` (Elo-like rating)
  - `ping`
  - `region`
- Priority of constraints, in order of importance:
  1. Region (strict initially, cross-region only as a last resort).
  2. Elo / MMR (balanced teams, limited deviation per player).
  3. Ping (lower is better; limits relax over time).
  4. Time in queue (older players get more relaxed constraints).
- Constraints start strict and gradually relax as players wait longer, eventually allowing cross-region matches for long-waiting players.

The server records basic match statistics to an append-only `matches.jsonl` file for later inspection and analysis.

## Architecture

- **Protocol Buffers / gRPC**
  - `proto/matchmaker.proto` defines:
    - `Enqueue(Player) -> EnqueueResponse`
    - `Cancel(PlayerID) -> CancelResponse`
    - `StreamMatches(PlayerID) -> stream Match`
  - Code is generated into the `generated/` folder.

- **Server (`matchmaker_server`)**
  - Listens on a configurable gRPC address (e.g. `0.0.0.0:50051`).
  - Implements the `Matchmaker` service using a matchmaking engine that:
    - Maintains per-region queues of players.
    - Runs a background tick loop to build matches.
    - Forms 5v5 games that respect region, Elo, ping, and time-in-queue rules.
  - Persists match stats to `matches.jsonl`.

- **Simulator (`match_simulator`)**
  - Reads configuration from `config/sim_config.json` (arrival rate, Elo distributions, ping distributions, etc.).
  - Generates synthetic players (up to a few thousand concurrently in v1).
  - Sends `Enqueue` requests over gRPC to the server.
  - In v1, players play exactly one match and then disappear (no requeue yet).

## Configuration

Two separate JSON configuration files (planned):

- `config/server_config.json`
  - Regions
  - Tick interval
  - Elo and ping thresholds + relaxation rates
  - Cross-region wait threshold
  - Match stats output path

- `config/sim_config.json`
  - Total players or simulation duration
  - Players-per-second per region
  - MMR / Elo distribution
  - Ping distributions per region
  - Target server address

## CI/CD and Docker

Planned CI/CD setup:

- **CI (GitHub Actions)** on `ubuntu-latest`:
  - Configure and build both executables with CMake.
  - Run unit tests (e.g. GoogleTest) and simple performance benchmarks.

- **Docker**:
  - `Dockerfile.server` and `Dockerfile.simulator` build Linux-based images for the server and simulator.
  - A `docker-compose.yml` will start both containers together (server + simulator).

- **CD**:
  - On push to the main branch, CI builds and (optionally) publishes Docker images to a container registry.

This project is intentionally compact but structured like a real service, to demonstrate backend design, concurrency, gRPC usage, and basic CI/CD practices.
