# StrataDB

### A C++17 LSM-Tree Based Key-Value Storage Engine

StrataDB is a learning-focused key-value storage engine built from scratch in C++17, inspired by the architecture of storage engines such as RocksDB and LevelDB.

The project explores how modern storage engines manage in-memory data, persistent writes, sorted disk files, and efficient key lookups using fundamental data structures and storage-engine concepts.

**Repository:** [Amrittt22/StrataDB](https://github.com/Amrittt22/StrataDB)

---

## Project Status

**Current milestone: Day 2 – Core Storage Components Implemented**

- 54 unit tests passing.
- Skip List-based MemTable.
- Write-Ahead Log (WAL) with recovery.
- Immutable SSTable file writer and reader.
- Bloom Filter integrated into SSTable lookups.

The complete LSM-tree workflow, including automatic flushing, compaction, and the REST API, is still under development.

---

## Architecture

StrataDB is being developed around an LSM-tree (Log-Structured Merge-Tree) architecture.

```text
              Client Application
                      |
                      v
                Key-Value API
                 (Planned)
                      |
                      v
                +-----------+
                |  MemTable |
                | Skip List |
                +-----------+
                      |
             +--------+--------+
             |                 |
             v                 v
        Write-Ahead Log    Read Lookup
             |                 |
             v                 v
          Disk Log         SSTables
                              |
                       +------+------+
                       |             |
                       v             v
                  Bloom Filter   Binary Search
                       |
                       v
                  Key Retrieval
```

*Note: The complete read path and automatic MemTable-to-SSTable flushing are planned integration work.*

---

## Implemented Features

### 1. Skip List

A probabilistic ordered data structure used as the underlying storage structure for the MemTable.

- Insert key-value pairs.
- Search for keys.
- Delete keys.
- Update existing values.
- Ordered traversal of entries.
- Randomized node levels.

### 2. MemTable

An in-memory data structure built on top of the Skip List.

- Stores key-value pairs.
- Supports PUT, GET, and DELETE operations.
- Tracks the number of stored entries.
- Exposes sorted entries for SSTable generation.
- Integrates with the Write-Ahead Log.

### 3. Write-Ahead Log (WAL)

The WAL provides persistent operation logging and startup recovery.

- Appends PUT and DELETE records.
- Stores records in binary format.
- Replays operations during startup.
- Recovers the MemTable after a restart.
- Detects and truncates incomplete trailing records.

**Current limitation:** The implementation uses stream flushing but does not yet guarantee physical disk durability through OS-level synchronization such as `fsync` or `FlushFileBuffers`.

### 4. SSTable (Sorted String Table)

SSTables provide an immutable, sorted on-disk representation of key-value entries.

Implemented capabilities:

- Binary SSTable file format.
- Sorted key-value serialization.
- SSTable file loading.
- Key lookup using binary search.
- Validation of file headers and entry ordering.
- Handling of binary keys and values.
- Duplicate-key rejection during writing.

**Current implementation detail:** The reader loads the SSTable entries into memory. An on-disk sparse index and fully disk-backed lookup are planned improvements.

### 5. Bloom Filter

A probabilistic data structure used to avoid unnecessary SSTable lookups.

- Configurable expected number of entries.
- Configurable false-positive probability.
- Multiple hash functions.
- Fast membership checks.
- No false negatives for correctly inserted keys.

The Bloom Filter is currently built when an SSTable is loaded rather than persisted as a separate on-disk structure.

### 6. Testing

GoogleTest is used for unit testing the storage engine.

Current test coverage includes:

- Skip List operations.
- MemTable operations.
- WAL append and replay.
- Recovery after restart.
- Truncated WAL recovery.
- SSTable serialization and deserialization.
- Binary key-value handling.
- Bloom Filter membership checks.
- Invalid SSTable handling.

**Latest reported result: 54/54 tests passing.**

---

## Tech Stack

| Technology | Purpose |
|---|---|
| C++17 | Core storage engine |
| CMake | Build system |
| Ninja | Build execution |
| GoogleTest | Unit testing |
| STL | Data structures and utilities |
| Binary File I/O | Persistent storage |
| Git & GitHub | Version control |

---

## Project Structure

```text
StrataDB/
│
├── engine/
│   ├── include/
│   │   ├── SkipList.h
│   │   ├── MemTable.h
│   │   ├── Stats.h
│   │   ├── Logger.h
│   │   ├── WAL.h
│   │   ├── BloomFilter.h
│   │   └── SSTable.h
│   │
│   ├── src/
│   │   ├── main.cpp
│   │   ├── SkipList.cpp
│   │   ├── MemTable.cpp
│   │   ├── Stats.cpp
│   │   ├── Logger.cpp
│   │   ├── WAL.cpp
│   │   ├── BloomFilter.cpp
│   │   └── SSTable.cpp
│   │
│   └── tests/
│       ├── test_main.cpp
│       ├── skiplist_test.cpp
│       ├── memtable_test.cpp
│       ├── stats_test.cpp
│       ├── logger_test.cpp
│       ├── wal_test.cpp
│       ├── bloom_filter_test.cpp
│       └── sstable_test.cpp
│
├── benchmarks/
├── docs/
├── CMakeLists.txt
├── README.md
└── .gitignore
```

---

## Building and Running

### Prerequisites

- C++17 compatible compiler.
- CMake.
- Ninja (if using the Ninja generator).
- Git.

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

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

For the Windows MSYS2 UCRT64 environment, ensure the UCRT64 `bin` directory is first in your PATH before building and running tests.

---

## Development Roadmap

### Day 1 – In-Memory Core and Observability

- [x] CMake project setup.
- [x] Skip List implementation.
- [x] MemTable implementation.
- [x] Statistics tracking.
- [x] Console Logger.
- [x] Unit tests.

### Day 2 – Persistence and Storage Components

- [x] Write-Ahead Log.
- [x] WAL replay and recovery.
- [x] Sorted Skip List traversal.
- [x] SSTable writer.
- [x] SSTable reader.
- [x] Bloom Filter implementation.
- [x] Bloom Filter integration with SSTable lookup.
- [x] Unit tests for storage components.

### Day 3 – LSM Integration and Backend

- [ ] Automatic MemTable flushing.
- [ ] Safe WAL rotation and checkpointing.
- [ ] Tombstone-based deletion.
- [ ] Multiple SSTable management.
- [ ] Integrated read path across MemTable and SSTables.
- [ ] SSTable version resolution.
- [ ] SSTable compaction.
- [ ] Range scan support.
- [ ] REST API.

### Day 4 – Dashboard and Optimization

- [ ] React dashboard.
- [ ] Live MemTable statistics.
- [ ] SSTable visualization.
- [ ] Bloom Filter statistics.
- [ ] Engine event log.
- [ ] Benchmarking.
- [ ] Performance optimization.
- [ ] Documentation and demo.

---

## Concepts Explored

Through StrataDB, the project explores:

- Log-Structured Merge Trees (LSM Trees).
- Skip Lists and probabilistic data structures.
- Write-Ahead Logging and recovery.
- Immutable sorted storage files.
- Bloom Filters and probabilistic membership testing.
- Binary serialization and deserialization.
- C++ memory management and RAII.
- Unit testing and modular software design.
- Database storage-engine architecture.

---

## Future Improvements

- Persistent SSTable indexes.
- On-disk Bloom Filter metadata.
- Checksums and stronger corruption detection.
- Crash-safe WAL synchronization.
- Efficient disk-backed SSTable lookups.
- Multi-level compaction.
- Concurrent reads and writes.
- Performance benchmarking against standard data structures.

---

## Author

**Amritanshu Singh**

Computer Science Engineering Student  
Nitte Meenakshi Institute of Technology, Bengaluru

- GitHub: [@Amrittt22](https://github.com/Amrittt22)
- LinkedIn: [Amritanshu Kumar Singh](https://www.linkedin.com/in/amritanshu-kumar-singh-203254333/)

---

*StrataDB is an educational project developed to understand the internal architecture and engineering principles behind modern key-value storage engines.*