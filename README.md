# StrataDB

### A C++17 LSM-Tree Based Key-Value Storage Engine

StrataDB is a key-value storage engine being built from scratch in C++17, based on the **Log-Structured Merge-Tree (LSM-Tree)** architecture.

The goal is to understand and implement the core internals of modern storage engines, including memory-based indexing, write-ahead logging, sorted disk storage, Bloom filters, compaction, and crash recovery.

A React-based dashboard is also planned to visualize database operations and performance metrics.

---

## Project Status

**Currently in development — Day 1 completed.**

| Component | Status |
|---|---|
| Project setup with CMake | Implemented |
| Skip List | Implemented |
| MemTable | Implemented |
| Statistics module | Implemented |
| Console event logger | Implemented |
| Unit testing with GoogleTest | Implemented |
| Write-Ahead Log (WAL) | Planned |
| SSTables | Planned |
| Bloom Filters | Planned |
| Compaction | Planned |
| Crash Recovery | Planned |
| Range Queries | Planned |
| HTTP API | Planned |
| React Dashboard | Planned |
| Performance Benchmarks | Planned |

## Architecture

StrataDB is designed around an LSM-Tree storage architecture.

```text
             Client Applications
                     |
                     v
              HTTP API Layer
                     |
                     v
               MemTable
              (Skip List)
                     |
                     v
             Write-Ahead Log
                     |
                     v
             Immutable MemTable
                     |
                     v
                SSTables
              (Disk Storage)
                     |
              +------+------+
              |             |
              v             v
          Bloom Filter   Compaction
              |
              v
          Read Operations
```

*Note: This represents the planned architecture. Persistent storage components are still under development.*

## Implemented Features

### 1. Skip List

A probabilistic data structure used as the indexing mechanism for the MemTable.

- Ordered key-value storage.
- Efficient insertion and search.
- Key deletion.
- Updating existing keys.
- Randomized node levels.

### 2. MemTable

An in-memory key-value storage layer built on top of the custom Skip List.

- Insert key-value pairs.
- Retrieve values using keys.
- Update existing entries.
- Delete entries.
- Track the number of entries.

### 3. Statistics Module

Tracks database operation metrics.

- Total PUT operations.
- Total GET operations.
- Total DELETE operations.
- Successful GET operations.
- Failed GET operations.
- Total operation count.

### 4. Event Logger

A lightweight console logging system with three logging levels:

- INFO
- WARNING
- ERROR

### 5. Unit Testing

GoogleTest is used to validate the core data structures and components.

The current test suite covers Skip List, MemTable, Statistics, and Logger functionality.

## Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Core storage engine |
| CMake | Build system |
| Ninja | Build generator |
| GoogleTest | Unit testing |
| React | Planned dashboard |
| Vite | Planned frontend tooling |
| Tailwind CSS | Planned dashboard styling |
| Chart.js | Planned performance visualizations |

## Project Structure

```text
StrataDB/
│
├── engine/
│   ├── include/
│   │   ├── SkipList.h
│   │   ├── MemTable.h
│   │   ├── Stats.h
│   │   └── Logger.h
│   │
│   ├── src/
│   │   ├── main.cpp
│   │   ├── SkipList.cpp
│   │   ├── MemTable.cpp
│   │   ├── Stats.cpp
│   │   └── Logger.cpp
│   │
│   └── tests/
│       ├── test_main.cpp
│       ├── skiplist_test.cpp
│       ├── memtable_test.cpp
│       ├── stats_test.cpp
│       └── logger_test.cpp
│
├── benchmarks/
├── docs/
├── CMakeLists.txt
└── README.md
```

## Getting Started

### Prerequisites

- C++17 compatible compiler
- CMake
- Ninja build system
- Git

### Clone the Repository

```bash
git clone https://github.com/Amrittt22/StrataDB.git
cd StrataDB
```

### Configure the Project

```bash
cmake -S . -B build -G Ninja
```

### Build

```bash
cmake --build build
```

### Run the Database

On Windows:

```powershell
.\build\stratadb.exe
```

### Run Unit Tests

```bash
ctest --test-dir build --output-on-failure
```

GoogleTest is fetched through CMake FetchContent during configuration.

## Development Roadmap

### Phase 1 — In-Memory Storage Foundation
- [x] CMake project setup
- [x] Skip List implementation
- [x] MemTable implementation
- [x] Statistics module
- [x] Event logger
- [x] Unit tests

### Phase 2 — Persistent Storage
- [ ] Write-Ahead Log
- [ ] SSTable implementation
- [ ] Bloom Filters
- [ ] Crash Recovery

### Phase 3 — Storage Engine Operations
- [ ] Read path optimization
- [ ] Tombstone-based deletion
- [ ] Compaction
- [ ] Range queries

### Phase 4 — API and Dashboard
- [ ] HTTP server
- [ ] REST API endpoints
- [ ] React dashboard
- [ ] Live operation metrics
- [ ] Performance charts

### Phase 5 — Benchmarking and Optimization
- [ ] Read/write benchmarks
- [ ] Performance comparisons
- [ ] Memory usage analysis
- [ ] Documentation and