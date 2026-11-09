# StrataDB

**A C++ LSM-Tree-Based Key-Value Storage Engine**

StrataDB is a lightweight key-value storage engine built from scratch in C++. It is designed to explore the internal workings of modern storage systems, including Log-Structured Merge Trees (LSM-Trees), Write-Ahead Logging (WAL), MemTables, SSTables, Bloom Filters, compaction, checkpoint recovery, and REST API integration.

The project combines low-level C++ storage-engine concepts with a backend interface that allows external applications to interact with the database.

> **Project status:** Core storage engine and C++ REST API implemented. React dashboard is the next development milestone.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Storage Engine Components](#storage-engine-components)
- [Data Flow](#data-flow)
- [REST API](#rest-api)
- [Build and Run](#build-and-run)
- [Testing](#testing)
- [Current Progress](#current-progress)
- [Roadmap](#roadmap)
- [Technology Stack](#technology-stack)

---

## Overview

StrataDB is a persistent key-value database that uses an LSM-Tree-inspired architecture.

Instead of modifying disk-based data structures for every write, StrataDB first records writes in a Write-Ahead Log and stores them in an in-memory MemTable. When the MemTable reaches its configured threshold, its contents are flushed into an immutable Sorted String Table (SSTable).

The engine uses sequence numbers to identify the newest version of a key, allowing it to resolve updates and deletions across multiple storage files.

StrataDB also provides a C++ HTTP REST API, making it possible to interact with the engine through external applications.

## Features

### Storage Engine

- Persistent key-value storage.
- In-memory MemTable.
- Skip List-based data organization.
- Write-Ahead Logging (WAL).
- Immutable SSTable files.
- Bloom Filter-based lookup optimization.
- Sequence-number-based record versioning.
- Tombstone-based deletion.
- Range scanning.
- SSTable compaction.
- Automatic MemTable flushing.

### Recovery and Persistence

- Sequence-aware WAL records.
- WAL replay during database initialization.
- SSTable V3 format with sequence numbers.
- Manifest-based checkpoint tracking.
- Checkpoint-aware WAL recovery.
- MemTable clearing after successful checkpoint progression.
- Data persistence across server restarts.

### Backend API

- HTTP REST API implemented in C++.
- Health-check endpoint.
- PUT, GET, and DELETE operations.
- Range scan endpoint.
- Storage statistics endpoint.
- Request serialization through a mutex in the HTTP server.

---

## Architecture

StrataDB follows an LSM-Tree-inspired storage architecture.

```text
                 Client Application
                         |
                         v
                  HTTP REST API
                         |
                         v
                    Database
                         |
              +----------+----------+
              |                     |
              v                     v
          MemTable                 WAL
          Skip List          Write-Ahead Log
              |
              | Automatic / Manual Flush
              v
          SSTable Files
          Immutable Storage
              |
              v
          SSTable Manager
              |
              +----------------+
              |                |
              v                v
          Bloom Filters     Compaction
              |
              v
        Persistent Storage

          Manifest
     Checkpoint Metadata
```

## Storage Engine Components

### 1. MemTable

The MemTable is the in-memory component responsible for holding recent writes.

StrataDB uses a Skip List to organize entries and support efficient lookup and ordered traversal.

Responsibilities:

- Store recent key-value updates.
- Maintain sequence numbers.
- Track deleted keys using tombstones.
- Provide ordered snapshots for flushing.
- Trigger automatic flushing when the configured threshold is reached.

### 2. Write-Ahead Log (WAL)

The WAL records database modifications before they are applied to the in-memory data structure.

Each WAL record contains:

- Operation type.
- Sequence number.
- Key length.
- Value length.
- Key.
- Value.

The WAL supports replay during recovery and checkpoint-aware skipping of already checkpointed records.

### 3. SSTable

An SSTable is an immutable, disk-based file containing sorted database entries.

StrataDB currently uses the V3 SSTable format, which stores sequence numbers alongside keys, values, and tombstone information.

This allows the engine to determine which version of a key is the newest across multiple SSTables.

### 4. Bloom Filter

Bloom Filters help reduce unnecessary SSTable lookups.

They provide probabilistic membership checks:

- A negative result means the key is definitely absent.
- A positive result means the key may be present.

### 5. SSTable Manager

The SSTable Manager manages the collection of immutable SSTable files.

Responsibilities:

- Track available SSTables.
- Search across SSTables.
- Resolve record versions using sequence numbers.
- Perform compaction.
- Preserve the newest record for each key during compaction.

### 6. Compaction

Compaction merges SSTables into a consolidated representation.

StrataDB uses sequence numbers to select the newest version of each key. Tombstones are retained during the current compaction process so that deleted keys do not incorrectly reappear from older tables.

### 7. Manifest and Checkpoint Recovery

The Manifest stores the database checkpoint sequence number.

During recovery:

1. The database loads the checkpoint metadata.
2. SSTable storage is initialized.
3. WAL records are examined.
4. Records at or below the checkpoint are skipped.
5. Newer records are replayed into the MemTable.

This prevents already checkpointed operations from being replayed as new writes.

---

## Data Flow

### Write Operation

```text
Client
  |
  v
REST API
  |
  v
Database::put()
  |
  v
Append record to WAL
  |
  v
Insert into MemTable
  |
  v
Check MemTable threshold
  |
  +---- Threshold reached? ----+
  |                            |
  No                           Yes
  |                            |
  v                            v
Return success            Flush to SSTable
                               |
                               v
                         Save checkpoint
                               |
                               v
                         Clear MemTable
```

### Read Operation

```text
Client
  |
  v
REST API
  |
  v
Database::get()
  |
  v
Check MemTable
  |
  +---- Found? ----+
  |                |
 Yes               No
  |                |
  v                v
Return value   Search SSTables
                    |
                    v
              Bloom Filter
                    |
                    v
              Read candidate
                    |
                    v
          Resolve newest sequence
                    |
                    v
              Return result
```

### Delete Operation

StrataDB represents deletions using tombstones.

A delete operation records a tombstone with a sequence number. The tombstone ensures that older versions of the key are not returned during reads or recovery.

---

## REST API

StrataDB exposes an HTTP interface through its C++ server.

**Default server address:**

`http://localhost:8080`

| Method | Endpoint | Description |
|---|---|---|
| GET | `/health` | Check server health |
| PUT | `/kv/:key` | Insert or update a key |
| GET | `/kv/:key` | Retrieve a key |
| DELETE | `/kv/:key` | Delete a key |
| GET | `/scan?start=a&end=z` | Scan a key range |
| GET | `/stats` | Retrieve storage statistics |

### API Examples

**Health check**

```http
GET /health
```

**Insert a value**

```http
PUT /kv/name
Content-Type: text/plain

Amritanshu
```

**Retrieve a value**

```http
GET /kv/name
```

**Update a value**

```http
PUT /kv/name
Content-Type: text/plain

StrataDB
```

**Delete a key**

```http
DELETE /kv/name
```

**Scan a range**

```http
GET /scan?start=a&end=z
```

**Retrieve statistics**

```http
GET /stats
```

---

## Build and Run

### Prerequisites

- C++ compiler with C++17 support.
- CMake.
- Ninja build system.
- Git.
- Internet access during initial configuration if the HTTP library needs to be downloaded.

The current development environment uses MSYS2 UCRT64 GCC, CMake, and Ninja on Windows.

### 1. Clone the repository

```bash
git clone https://github.com/Amrittt22/StrataDB.git
cd StrataDB
```

### 2. Configure the project

For the current Windows MSYS2 UCRT64 environment, open PowerShell:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

cmake -S . -B build -G Ninja `
  -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe"
```

### 3. Build

```powershell
cmake --build build
```

### 4. Run the REST server

```powershell
.\build\stratadb_server.exe
```

The server starts on port `8080` and uses `./stratadb_data` as its default database directory.

You can also specify a custom data directory and port:

```powershell
.\build\stratadb_server.exe .\my_database 8080
```

---

## Testing

StrataDB includes GoogleTest-based tests for its storage-engine components.

The test suite covers components including:

- Bloom Filter.
- Database operations.
- Logger.
- MemTable.
- Skip List.
- SSTable.
- SSTable Manager.
- Statistics.
- Write-Ahead Log.

The development process has also included tests for sequence-aware recovery, checkpoint handling, automatic flushing, and persistence across restarts.

### REST API Verification

The HTTP API has been manually tested using PowerShell.

Example:

```powershell
$base = "http://localhost:8080"

Invoke-RestMethod "$base/health"

Invoke-RestMethod -Method PUT `
  -Uri "$base/kv/name" `
  -Body "Amritanshu"

Invoke-RestMethod "$base/kv/name"

Invoke-RestMethod -Method DELETE `
  -Uri "$base/kv/name"
```

Restart persistence has also been tested by writing a key, stopping the server, restarting it, and retrieving the stored value.

---

## Current Progress

<details>
<summary><strong>Completed — Core Storage Engine</strong></summary>

- LSM-Tree-inspired storage architecture.
- Skip List-based MemTable.
- Write-Ahead Logging.
- SSTable storage.
- Bloom Filter integration.
- Basic database operations.
- Range scanning.
- SSTable compaction.

</details>

<details>
<summary><strong>Completed — Recovery and Optimization</strong></summary>

- Sequence-aware WAL.
- SSTable V3 with sequence numbers.
- Sequence-based version resolution.
- Tombstone handling.
- Manifest and checkpoint tracking.
- Checkpoint-aware recovery.
- Automatic MemTable flushing.

</details>

<details>
<summary><strong>Completed — HTTP Backend</strong></summary>

- C++ REST API server.
- Health endpoint.
- Key-value CRUD endpoints.
- Range scan endpoint.
- Statistics endpoint.
- Manual API testing.
- Restart persistence verification.

</details>

<details>
<summary><strong>Upcoming — React Dashboard</strong></summary>

- React frontend.
- Backend API integration.
- Database health indicator.
- Key-value editor.
- Range scanner interface.
- Storage statistics dashboard.
- End-to-end frontend/backend testing.

</details>

---

## Roadmap

| Milestone | Work | Status |
|---|---|---|
| Day 1–2 | Core storage engine and LSM components | Completed |
| Day 3 | Sequence-aware storage, recovery, auto-flush, REST API | Completed |
| Day 4 | React dashboard and API integration | Upcoming |
| Day 5+ | Integration testing and reliability improvements | Planned |
| Final | Documentation, deployment, and project polish | Planned |

The roadmap is subject to change as the implementation evolves.

---

## Technology Stack

| Technology | Purpose |
|---|---|
| C++ | Storage engine and HTTP backend |
| C++17 | Language standard |
| CMake | Build configuration |
| Ninja | Build system |
| GoogleTest | Unit testing |
| cpp-httplib | HTTP server |
| React | Planned frontend |
| JavaScript / TypeScript | Planned frontend development |

---

## Engineering Notes and Current Limitations

StrataDB is an educational and portfolio-oriented storage-engine project. Its current implementation demonstrates important database concepts, but it should not yet be treated as a production database.

Areas for further engineering work include:

- Stronger operating-system-level durability guarantees.
- More extensive crash and recovery testing.
- Compaction crash consistency and orphan-file cleanup.
- Manifest and SSTable consistency validation.
- Concurrency control and stress testing.
- More comprehensive REST API error handling.
- Authentication, TLS, and restricted CORS before exposing the API publicly.
- Performance benchmarks against existing storage engines.

These are intentional future engineering goals rather than claims of functionality already implemented.

---

## Author

**Amritanshu Singh**

- GitHub: [Amrittt22](https://github.com/Amrittt22)
- Project: [StrataDB](https://github.com/Amrittt22/StrataDB)

---

*StrataDB — Exploring how storage engines work, one component at a time.*