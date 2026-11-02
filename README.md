# EmberDB

**A C++17 LSM-Tree Key-Value Store with a Live React Dashboard**

EmberDB is a learning-focused database systems project that aims to build a key-value storage engine from scratch using C++17. Inspired by storage engines such as RocksDB and LevelDB, it explores how databases manage data in memory, persist it to disk, recover from crashes, and optimize read and write operations.

The project will include a live React dashboard that visualizes storage-engine internals, database operations, performance metrics, and recovery events.

> **Status:** Planning and initial setup. Core storage-engine components are not implemented yet.

## Project Goals

- Build a key-value storage engine from scratch in C++17.
- Understand LSM-Tree architecture and storage-engine internals.
- Implement persistent storage and crash recovery.
- Explore efficient lookups using Bloom filters.
- Implement SSTable merging through compaction.
- Visualize internal database operations through a live dashboard.
- Benchmark performance and test correctness.

## Planned Architecture

```text
                 React + Vite Dashboard
                          |
                     REST API
                          |
                 C++ HTTP Server
                          |
                  EmberDB Engine
                          |
          +---------------+---------------+
          |               |               |
       MemTable           WAL          SSTables
       Skip List       Recovery      Sorted Files
          |                               |
          +---------------+---------------+
                          |
                   Bloom Filters
                          |
                      Compaction
```

The dashboard will communicate with the C++ engine through a REST API, keeping the user interface separate from the storage-engine implementation.

## Planned Features

- **Key-Value Operations:** Insert, retrieve, update, and delete records.
- **Skip List MemTable:** Maintain recently written records in sorted in-memory order.
- **Write-Ahead Log (WAL):** Support recovery by replaying logged operations after a crash.
- **SSTables:** Persist sorted records in immutable on-disk files.
- **Bloom Filters:** Reduce unnecessary SSTable lookups.
- **Compaction:** Merge SSTables and clean up obsolete versions and tombstones.
- **Range Scans:** Retrieve records over a key range in sorted order.
- **Live Dashboard:** Monitor MemTable usage, SSTables, engine statistics, and events.
- **Crash Recovery Demo:** Demonstrate data recovery after an unclean shutdown.
- **Benchmarking:** Measure throughput and compare results against a `std::map` baseline.

## Tech Stack

| Component | Technology |
|---|---|
| Storage engine | C++17 |
| Build system | CMake |
| Unit testing | GoogleTest |
| HTTP server | cpp-httplib |
| JSON serialization | nlohmann/json |
| Frontend | React + Vite |
| Styling | Tailwind CSS |
| Charts | Chart.js |
| Version control | Git and GitHub |

## Development Roadmap

The project is planned in four milestones.

### Phase 1 — In-Memory Core
- Set up the project structure and build system.
- Implement the skip list and MemTable.
- Add memory usage tracking, statistics, and an event log.
- Write unit tests.

### Phase 2 — Persistence and Recovery
- Implement the Write-Ahead Log.
- Create SSTables and a sparse index.
- Add Bloom filters.
- Implement and test crash recovery.

### Phase 3 — Reads and Compaction
- Implement the complete read path.
- Add range scans.
- Implement single-level SSTable compaction.
- Expose database operations through a REST API.

### Phase 4 — Dashboard and Benchmarking
- Build the React dashboard.
- Visualize storage statistics and engine events.
- Add crash-recovery controls.
- Implement benchmarks and document results.

## Project Structure

The repository will evolve toward the following structure as development progresses.

```text
EmberDB/
├── include/
│   └── emberdb/
├── src/
├── server/
├── dashboard/
├── tests/
├── docs/
├── scripts/
├── CMakeLists.txt
├── .gitignore
└── README.md
```

## Testing and Benchmarks

Correctness will be validated with unit and integration tests covering key-value operations, persistence, recovery, Bloom filters, range scans, and compaction.

Performance benchmarks will measure operations per second and compare equivalent workloads against a `std::map` baseline. Results and test conditions will be documented once the implementation is available.

## Learning Objectives

Through EmberDB, I aim to deepen my understanding of:

- Data structures and algorithms used in storage systems.
- File I/O and persistent data formats.
- Database indexing and query paths.
- Crash recovery and data durability.
- Storage-engine performance and trade-offs.
- C++ systems programming and testing.
- Full-stack integration between C++ services and React.

## Current Status

EmberDB is currently in the planning stage. Features listed above represent the intended scope and are not yet claimed as implemented.

## License

This project is intended to be released under the MIT License. A license will apply once the corresponding license file is added to the repository.
