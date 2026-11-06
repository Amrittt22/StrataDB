# StrataDB

### A C++17 LSM-Tree-Based Key-Value Storage Engine with a Live React Dashboard

StrataDB is a from-scratch key-value storage engine written in **C++17**, inspired by the internal architecture of modern storage systems such as RocksDB and LevelDB.

The goal is to understand and implement how database storage engines manage writes, reads, persistence, indexing, and recovery — rather than relying on an existing database.

StrataDB will use a **Log-Structured Merge Tree (LSM-Tree)** architecture to organize data efficiently, with a React-based dashboard to visualize its internal operations in real time.

> **Status:** Planning and development preparation. Features listed below are planned and will be updated as implementation progresses.

---

## Core Features

The planned implementation includes:

- **Key-Value Operations:** Insert, retrieve, update, and delete key-value pairs.
- **MemTable:** In-memory storage implemented using a Skip List.
- **Write-Ahead Log (WAL):** Record writes before applying them to memory for durability.
- **SSTables:** Store sorted key-value data on disk.
- **Bloom Filters:** Reduce unnecessary disk lookups.
- **Compaction:** Merge SSTables and manage obsolete entries.
- **Range Queries:** Retrieve keys within a specified range.
- **Crash Recovery:** Restore database state using the WAL.
- **Performance Benchmarking:** Compare operations against `std::map`.
- **Live Dashboard:** Visualize storage structures, operations, and performance metrics.

---

## Architecture

```text
                  React + Vite Dashboard
                           |
                           | HTTP / JSON
                           |
                  C++ HTTP API Server
                           |
                           v
                  StrataDB Storage Engine
                           |
              +------------+------------+
              |            |            |
           MemTable       WAL        Read Path
          (Skip List)   (Durability)    |
              |                         |
              v                         v
         Flush to Disk              Bloom Filter
              |                         |
              v                         v
           SSTables <-------------- SSTable Lookup
              |
              v
          Compaction
              |
              v
         Updated SSTables
```

---

## Technology Stack

| Component | Technology |
|---|---|
| Core Engine | C++17 |
| Build System | CMake |
| Testing | GoogleTest |
| In-memory Structure | Skip List |
| Persistence | Write-Ahead Log |
| Disk Storage | SSTables |
| Filtering | Bloom Filters |
| HTTP Server | cpp-httplib |
| JSON Serialization | nlohmann/json |
| Frontend | React + Vite |
| Styling | Tailwind CSS |
| Charts | Chart.js |
| Version Control | Git & GitHub |

---

## Planned Dashboard

The React dashboard will provide visibility into the internal working of StrataDB.

| Panel | Purpose |
|---|---|
| Key-Value Console | Execute PUT, GET, and DELETE operations |
| Latency Monitor | Display operation execution times |
| MemTable Monitor | Visualize current in-memory entries |
| SSTable Explorer | Inspect on-disk sorted tables |
| Bloom Filter Stats | Display filter statistics |
| Event Log | Monitor database events |
| Crash Recovery | Simulate crashes and recovery |
| Benchmark Runner | Compare StrataDB against `std::map` |

---

## Development Roadmap

### Phase 1 — In-Memory Storage Engine
- Configure CMake and GoogleTest.
- Implement a Skip List.
- Build the MemTable.
- Implement basic key-value operations.
- Introduce statistics and event logging.

### Phase 2 — Persistence and Recovery
- Implement the Write-Ahead Log.
- Implement SSTable creation and storage.
- Add sparse indexing.
- Implement Bloom Filters.
- Build crash recovery.

### Phase 3 — Read Path and Compaction
- Implement the complete read path.
- Add SSTable lookups.
- Implement range scans.
- Build single-level compaction using a k-way merge.

### Phase 4 — API and Dashboard
- Build the HTTP API using cpp-httplib.
- Connect the API to the storage engine.
- Develop the React + Vite dashboard.
- Visualize database statistics and events.
- Add benchmarking and performance charts.

---

## Project Structure

The following is the intended project structure and may evolve during development.

```text
StrataDB/
│
├── engine/
│   ├── include/
│   ├── src/
│   └── tests/
│
├── server/
│   ├── include/
│   └── src/
│
├── dashboard/
│   ├── src/
│   ├── public/
│   └── package.json
│
├── benchmarks/
│
├── docs/
│
├── CMakeLists.txt
├── .gitignore
└── README.md
```

---

## Learning Objectives

Building StrataDB will involve exploring:

- Database storage engine architecture.
- Log-Structured Merge Trees.
- In-memory indexing and Skip Lists.
- Disk-based data structures.
- Write durability and crash recovery.
- Bloom filters and probabilistic data structures.
- Compaction and data organization.
- C++ systems programming.
- HTTP API design and frontend integration.
- Performance benchmarking.

---

## Project Goals

StrataDB is intended to be an educational yet technically substantial implementation of a storage engine.

The focus is on understanding how data moves from memory to disk, how reads are served efficiently, and how a database recovers after failure.

The dashboard will make these otherwise hidden internal operations observable.

---

## License

License to be decided.
