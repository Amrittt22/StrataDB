# StrataDB

### A C++ LSM-Tree-Based Key-Value Storage Engine

StrataDB is a lightweight persistent key-value storage engine built from scratch in C++. It explores the internal architecture of modern storage systems through concepts such as Log-Structured Merge Trees (LSM-Trees), Write-Ahead Logging (WAL), MemTables, SSTables, Bloom Filters, tombstones, compaction, and checkpoint-based recovery.

The project also includes a C++ HTTP REST API and a React dashboard, allowing users to interact with the storage engine through a graphical interface.

> **Project status:** Core storage engine, recovery mechanisms, automatic flushing, HTTP REST API, and React dashboard implemented and integration-tested.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Storage Engine Components](#storage-engine-components)
- [Data Flow](#data-flow)
- [Project Structure](#project-structure)
- [Technology Stack](#technology-stack)
- [Build and Run](#build-and-run)
- [REST API](#rest-api)
- [React Dashboard](#react-dashboard)
- [Testing](#testing)
- [Current Limitations](#current-limitations)
- [Future Improvements](#future-improvements)
- [Author](#author)

---

## Overview

StrataDB uses an LSM-Tree-inspired architecture to store key-value records persistently.

Instead of modifying disk-based data structures for every write, StrataDB first records modifications in a Write-Ahead Log and stores them in an in-memory MemTable.

When the MemTable reaches its configured threshold, its contents are flushed into an immutable Sorted String Table (SSTable).

Sequence numbers allow the engine to distinguish between different versions of a key. Tombstones represent deletions and prevent older versions from incorrectly reappearing.

The project provides:

- A persistent C++ key-value storage engine.
- An HTTP REST API implemented in C++.
- A React-based database management dashboard.
- Automatic MemTable flushing.
- Manifest-based checkpoint recovery.

---

## Features

### Storage Engine

- Persistent key-value storage.
- Skip List-based MemTable.
- Write-Ahead Logging (WAL).
- Immutable SSTable files.
- Bloom Filter-based lookup optimization.
- Sequence-number-based record versioning.
- Tombstone-based deletion.
- Ordered range scanning.
- SSTable compaction.
- Automatic and manual MemTable flushing.

### Recovery and Persistence

- Sequence-aware WAL records.
- WAL replay during database initialization.
- SSTable V3 format with sequence numbers.
- Manifest-based checkpoint tracking.
- Checkpoint-aware WAL recovery.
- Truncation of incomplete WAL tails during replay.
- Persistence across server restarts.

### Backend API

- C++ HTTP REST server using cpp-httplib.
- Health-check endpoint.
- PUT, GET, and DELETE operations.
- Range scan endpoint.
- Storage statistics endpoint.
- Configurable automatic flush threshold.
- Mutex-based request serialization.

### Frontend Dashboard

- React and TypeScript interface.
- Backend connection status.
- Key-value insertion and retrieval.
- Key updates and deletion.
- Range scanning interface.
- MemTable and SSTable statistics.
- Automatic statistics refresh.
- Success and error notifications.

---

## Architecture

StrataDB follows an LSM-Tree-inspired architecture.

```text
                  Client Application
                          |
                          v
                   React Dashboard
                          |
                          v
                    HTTP REST API
                          |
                          v
                       Database
                          |
              +-----------+-----------+
              |                       |
              v                       v
           MemTable                  WAL
           Skip List           Write-Ahead Log
              |
              | Automatic / Manual Flush
              v
          SSTable Files
        Immutable Storage
              |
              v
         SSTable Manager
              |
       +------+------+
       |             |
       v             v
   Bloom Filters  Compaction
              |
              v
       Persistent Storage

            Manifest
       Checkpoint Metadata
```

### Write path

1. A client sends a write request through the REST API.
2. The database assigns a sequence number.
3. The operation is appended to the WAL.
4. The record is inserted into the MemTable.
5. The engine checks the configured MemTable threshold.
6. If the threshold is reached, the MemTable is flushed into an SSTable.
7. The checkpoint metadata is updated.
8. The MemTable and WAL are cleared after successful checkpoint progression.

### Read path

1. The database checks the MemTable.
2. If the key is not found there, it searches the SSTables.
3. Bloom Filters help eliminate unnecessary SSTable lookups.
4. Candidate records are compared using sequence numbers.
5. The newest record determines the result.
6. A newest-version tombstone means the key is deleted.

---

## Storage Engine Components

### 1. MemTable

The MemTable is the in-memory component responsible for holding recent database modifications.

StrataDB uses a Skip List to maintain entries in sorted order.

Responsibilities:

- Store recent key-value updates.
- Maintain sequence numbers.
- Track deleted keys using tombstones.
- Provide ordered snapshots for flushing.
- Trigger automatic flushing when the configured threshold is reached.

### 2. Write-Ahead Log (WAL)

The WAL records database modifications before they are applied to the in-memory data structure.

Each WAL V2 record contains:

| Field | Description |
|---|---|
| Operation | Put or delete |
| Sequence number | Version identifier |
| Key length | Length of the key |
| Value length | Length of the value |
| Key | Key bytes |
| Value | Value bytes |

The WAL supports replay during recovery and skips records already covered by the manifest checkpoint.

### 3. SSTable

An SSTable is an immutable, disk-based file containing sorted database entries.

StrataDB currently uses the V3 SSTable format, identified by the `STRATAS3` magic value.

Entries contain:

- Key.
- Value.
- Tombstone information.
- Sequence number.

This allows the engine to resolve different versions of a key across multiple SSTables.

### 4. Bloom Filter

Bloom Filters reduce unnecessary SSTable lookups.

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

StrataDB uses sequence numbers to select the newest record for each key.

Tombstones are retained during the current compaction process to prevent deleted keys from reappearing from older tables.

### 7. Manifest and Checkpoint Recovery

The Manifest stores the database checkpoint sequence number.

During recovery:

1. The database loads checkpoint metadata.
2. SSTable storage is initialized.
3. WAL records are examined.
4. Records at or below the checkpoint are skipped.
5. Newer records are replayed into the MemTable.

This prevents already checkpointed operations from being replayed as new writes.

---

## Project Structure

```text
StrataDB/
│
├── engine/
│   ├── include/
│   │   ├── BloomFilter.h
│   │   ├── Database.h
│   │   ├── Logger.h
│   │   ├── Manifest.h
│   │   ├── MemTable.h
│   │   ├── Record.h
│   │   ├── SkipList.h
│   │   ├── SSTable.h
│   │   ├── SSTableManager.h
│   │   ├── Stats.h
│   │   └── WAL.h
│   │
│   ├── src/
│   │   ├── BloomFilter.cpp
│   │   ├── Database.cpp
│   │   ├── Logger.cpp
│   │   ├── Manifest.cpp
│   │   ├── MemTable.cpp
│   │   ├── SkipList.cpp
│   │   ├── SSTable.cpp
│   │   ├── SSTableManager.cpp
│   │   ├── Stats.cpp
│   │   ├── WAL.cpp
│   │   ├── main.cpp
│   │   └── server.cpp
│   │
│   └── tests/
│       ├── auto_flush_manual_test.cpp
│       ├── auto_flush_recovery_test.cpp
│       └── [component GoogleTest files]
│
├── frontend/
│   ├── public/
│   ├── src/
│   │   ├── api/
│   │   │   └── stratadb.ts
│   │   ├── assets/
│   │   ├── App.tsx
│   │   ├── App.css
│   │   ├── index.css
│   │   └── main.tsx
│   ├── .env.example
│   ├── package.json
│   └── vite.config.ts
│
├── CMakeLists.txt
├── .gitignore
└── README.md
```

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
| React | Frontend dashboard |
| TypeScript | Frontend application logic |
| Vite | Frontend development and build tooling |
| CSS | Dashboard styling |

---

## Build and Run

### Prerequisites

- C++ compiler with C++17 support.
- CMake.
- Ninja.
- Git.
- Node.js and npm.
- Internet access during initial dependency configuration if required.

The development environment used for this project is Windows with MSYS2 UCRT64 GCC, CMake, and Ninja.

### 1. Clone the repository

```bash
git clone https://github.com/Amrittt22/StrataDB.git
cd StrataDB
```

### 2. Configure the C++ backend

For Windows PowerShell with MSYS2 UCRT64:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

cmake -S . -B build -G Ninja `
  -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe"
```

### 3. Build the project

```powershell
cmake --build build
```

### 4. Run the REST server

```powershell
.\build\stratadb_server.exe
```

The server uses:

- Default host: `127.0.0.1`
- Default port: `8080`
- Default database directory: `./stratadb_data`
- Default MemTable flush threshold: `1000`

You can provide a custom database directory, port, and flush threshold:

```powershell
.\build\stratadb_server.exe .\my_database 8080 1000
```

The third argument controls the automatic MemTable flush threshold.

### 5. Run the React frontend

Open another terminal:

```powershell
cd frontend
npm install
```

Create a local `.env` file using `.env.example` as a reference:

```env
VITE_API_BASE_URL=http://localhost:8080
```

Start the frontend:

```powershell
npm run dev
```

Open the Vite URL displayed in the terminal, normally:

```text
http://localhost:5173
```

Keep the C++ backend and React development server running simultaneously.

---

## REST API

**Default server address:** `http://localhost:8080`

| Method | Endpoint | Description |
|---|---|---|
| GET | `/health` | Check server health |
| PUT | `/kv/:key` | Insert or update a key |
| GET | `/kv/:key` | Retrieve a key |
| DELETE | `/kv/:key` | Delete a key |
| GET | `/scan?start=a&end=z` | Scan a key range |
| GET | `/stats` | Retrieve storage statistics |

### API examples

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

Example statistics response:

```json
{
  "success": true,
  "memtable_size": 3,
  "sstable_count": 0,
  "auto_flush_success": true
}
```

The statistics endpoint exposes the current MemTable size, SSTable count, and last automatic flush status.

---

## React Dashboard

StrataDB includes a React and TypeScript dashboard connected to the C++ backend through the REST API.

The dashboard provides:

- Backend connection status.
- Database health information.
- Key-value insertion.
- Key retrieval.
- Key updates.
- Key deletion.
- Range scanning.
- MemTable size monitoring.
- SSTable count monitoring.
- Automatic flush status.
- Periodic statistics refresh.
- Success and error notifications.

The frontend communicates with the backend through the API client in:

```text
frontend/src/api/stratadb.ts
```

The backend base URL can be configured through `VITE_API_BASE_URL`.

---

## Testing

StrataDB includes GoogleTest-based tests covering its core storage-engine components.

The test suite includes coverage for:

- Bloom Filter.
- Database operations.
- Logger.
- MemTable.
- Skip List.
- SSTable.
- SSTable Manager.
- Statistics.
- Write-Ahead Log.
- Sequence-aware recovery.
- Checkpoint handling.
- Automatic flushing.

### Run the complete test suite

From the project root:

```powershell
ctest --test-dir build --output-on-failure
```

The Day 4 development checkpoint completed with:

**106 out of 106 tests passing.**

### Automatic flush tests

The project also includes standalone tests for automatic flushing and recovery:

```powershell
.\build\auto_flush_manual_test.exe
```

```powershell
.\build\auto_flush_recovery_test.exe
```

These tests exercise threshold-triggered flushing and persistence after reopening the database.

### REST API verification

The HTTP API was manually tested using PowerShell.

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

Restart persistence was also verified by writing a key, stopping the server, restarting it, and retrieving the stored value.

---

## Current Limitations

StrataDB is an educational and portfolio-oriented storage-engine project. It demonstrates important database concepts, but it should not yet be treated as a production database.

Current engineering limitations and areas requiring further validation include:

- Stronger operating-system-level durability guarantees.
- More extensive crash and recovery testing.
- Compaction crash consistency and orphan-file cleanup.
- Manifest and SSTable consistency validation.
- Concurrency control and stress testing.
- More comprehensive REST API error handling.
- Authentication and TLS.
- Restricted CORS before public exposure.
- Performance benchmarks against established storage engines.

The HTTP server currently serializes requests using a mutex, and its development CORS configuration should not be considered production-ready.

These limitations are documented as future engineering work, not as implemented capabilities.

---

## Future Improvements

Potential next development milestones include:

- Stronger crash consistency and durability.
- More robust manifest and SSTable validation.
- Improved compaction recovery.
- Concurrent request and stress testing.
- Storage-engine performance benchmarks.
- Production-oriented API security.
- Deployment and operational monitoring.

---

## Author

**Amritanshu Singh**

- GitHub: [Amrittt22](https://github.com/Amrittt22)
- Project Repository: [StrataDB](https://github.com/Amrittt22/StrataDB)

---

*StrataDB — Exploring how storage engines work, one component at a time.*
