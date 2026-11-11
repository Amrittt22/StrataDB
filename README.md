# StrataDB

### A C++ LSM-Tree-Based Persistent Key-Value Storage Engine

StrataDB is a persistent key-value storage engine built from scratch in C++17. It explores the internal architecture of modern storage systems through concepts such as Log-Structured Merge Trees (LSM-Trees), Write-Ahead Logging (WAL), MemTables, SSTables, Bloom Filters, tombstones, sequence numbers, compaction, and checkpoint-based recovery.

The project includes a C++ HTTP REST API, a React and TypeScript dashboard, and a standalone benchmarking executable.

**StrataDB is deployed with a React frontend on Vercel and a C++ backend on Railway, backed by persistent volume storage.**

<p align="center">
  <a href="https://frontend-amrit7.vercel.app/">
    <img src="https://img.shields.io/badge/Live%20Demo-StrataDB-red?style=for-the-badge" alt="Live Demo"/>
  </a>
  <a href="https://github.com/Amrittt22/StrataDB">
    <img src="https://img.shields.io/badge/GitHub-Repository-black?style=for-the-badge&logo=github" alt="GitHub"/>
  </a>
</p>

---

## Table of Contents

- [Live Demo](#live-demo)
- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Storage Engine Components](#storage-engine-components)
- [Data Flow](#data-flow)
- [Project Structure](#project-structure)
- [Technology Stack](#technology-stack)
- [Build and Run Locally](#build-and-run-locally)
- [REST API](#rest-api)
- [React Dashboard](#react-dashboard)
- [Deployment Architecture](#deployment-architecture)
- [Data Persistence](#data-persistence)
- [Testing](#testing)
- [Benchmarking](#benchmarking)
- [Current Limitations](#current-limitations)
- [Author](#author)

---

## Live Demo

**Live Dashboard:** https://frontend-amrit7.vercel.app/

**Backend API:** https://stratadb-production.up.railway.app

**Health Check:** https://stratadb-production.up.railway.app/health

**GitHub Repository:** https://github.com/Amrittt22/StrataDB

The live dashboard communicates with the deployed C++ backend through HTTP REST APIs.

Visitors can interact with the storage engine through the dashboard, including inserting, retrieving, updating, deleting, and scanning key-value records.

---

## Overview

StrataDB uses an LSM-Tree-inspired architecture to store key-value records persistently.

Instead of modifying disk-based data structures for every write, StrataDB first records modifications in a Write-Ahead Log and stores them in an in-memory MemTable.

When the MemTable reaches its configured threshold, its contents are flushed into an immutable Sorted String Table (SSTable).

Sequence numbers allow the engine to distinguish between different versions of a key. Tombstones represent deletions and prevent older versions from incorrectly reappearing.

The project provides:

- A persistent C++ key-value storage engine.
- A Skip List-based MemTable.
- Write-Ahead Logging and recovery.
- Immutable SSTable storage.
- Bloom Filter-based lookup optimization.
- Sequence-number-based record versioning.
- Tombstone-based deletion.
- Ordered range scanning.
- SSTable compaction.
- Manifest-based checkpoint recovery.
- Automatic MemTable flushing.
- A C++ HTTP REST API.
- A React and TypeScript dashboard.
- A standalone performance benchmark.
- A deployed application with persistent backend storage.

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
- Handling of incomplete WAL tails during replay.
- Persistence across server restarts.
- Persistent Railway volume mounted at `/data`.
- Verified data retention across backend restarts and new deployments.

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

### Testing and Benchmarking

- GoogleTest-based storage-engine test suite.
- WAL recovery regression tests.
- Compaction regression tests.
- HTTP API integration verification.
- Standalone benchmark executable.
- PUT, GET, range scan, and compaction measurements.
- CSV benchmark output.

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

### Write Path

1. A client sends a write request through the REST API.
2. The database assigns a sequence number.
3. The operation is appended to the WAL.
4. The record is inserted into the MemTable.
5. The engine checks the configured MemTable threshold.
6. If the threshold is reached, the MemTable is flushed into an SSTable.
7. Checkpoint metadata is updated.
8. The MemTable and WAL are cleared after successful checkpoint progression.

### Read Path

1. The database checks the MemTable.
2. If the key is not found there, it searches the SSTables.
3. Bloom Filters help eliminate unnecessary SSTable lookups.
4. Candidate records are compared using sequence numbers.
5. The newest record determines the result.
6. A newest-version tombstone means the key is deleted.

### Compaction Path

1. The SSTable Manager selects SSTables for compaction.
2. Their records are merged.
3. Sequence numbers are used to resolve different versions of the same key.
4. The newest record for each key is retained.
5. Tombstones are preserved by the current compaction implementation to prevent older values from reappearing.

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

## Data Flow

### Insertion

```text
Client
  |
  v
REST API
  |
  v
Database
  |
  +----> WAL
  |
  v
MemTable
  |
  v
Threshold Check
  |
  v
SSTable Flush
  |
  v
Persistent Storage
```

### Retrieval

```text
Client
  |
  v
REST API
  |
  v
Database
  |
  v
MemTable Lookup
  |
  v
SSTable Lookup
  |
  v
Bloom Filter
  |
  v
Sequence Number Resolution
  |
  v
Return Result
```

### Recovery

```text
Database Startup
      |
      v
Load Manifest
      |
      v
Initialize SSTables
      |
      v
Read WAL
      |
      v
Check Sequence Numbers
      |
      v
Replay Newer Records
      |
      v
Recovered Database State
```

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
│   ├── tests/
│   │   └── GoogleTest component and regression tests
│   │
│   └── benchmarks/
│       └── benchmark.cpp
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
├── Dockerfile
├── .dockerignore
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
| GoogleTest | Unit and regression testing |
| cpp-httplib | HTTP server |
| React | Frontend dashboard |
| TypeScript | Frontend application logic |
| Vite | Frontend development and build tooling |
| CSS | Dashboard styling |
| Docker | Backend containerization |
| Railway | C++ backend deployment and persistent storage |
| Vercel | Frontend hosting |

---

## Build and Run Locally

### Prerequisites

- C++ compiler with C++17 support.
- CMake.
- Ninja.
- Git.
- Node.js and npm.
- Internet access during initial dependency configuration if required.

The development environment used for this project is Windows with MSYS2 UCRT64 GCC, CMake, and Ninja.

### 1. Clone the Repository

```bash
git clone https://github.com/Amrittt22/StrataDB.git
cd StrataDB
```

### 2. Configure the C++ Backend

For Windows PowerShell with MSYS2 UCRT64:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"

cmake -S . -B build -G Ninja `
  -DCMAKE_CXX_COMPILER="C:/msys64/ucrt64/bin/g++.exe"
```

### 3. Build the Project

```powershell
cmake --build build
```

### 4. Run the REST Server

```powershell
.\build\stratadb_server.exe
```

The server uses:

| Setting | Default |
|---|---|
| Host | `127.0.0.1` |
| Port | `8080` |
| Database directory | `./stratadb_data` |
| MemTable flush threshold | `1000` |

You can provide a custom database directory, port, and flush threshold:

```powershell
.\build\stratadb_server.exe .\my_database 8080 1000
```

The third argument controls the automatic MemTable flush threshold.

### 5. Run the React Frontend

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

**Deployed server address:** `https://stratadb-production.up.railway.app`

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

The backend base URL is configured through `VITE_API_BASE_URL`.

### Live Dashboard

Try the deployed application:

https://frontend-amrit7.vercel.app/

---

## Deployment Architecture

StrataDB is deployed using separate frontend and backend hosting services.

```text
                  User Browser
                       |
                       v
              Vercel Frontend
             React + TypeScript
                       |
                       | HTTPS REST API
                       v
              Railway Backend
                  C++ Server
                       |
                       v
                  Database
                       |
          +------------+------------+
          |                         |
          v                         v
       MemTable                    WAL
          |
          v
       SSTables
          |
          v
    Railway Persistent Volume
             /data
```

### Frontend Deployment — Vercel

The React and TypeScript dashboard is hosted on Vercel.

**Live URL:**

https://frontend-amrit7.vercel.app/

The frontend uses the following environment variable:

```env
VITE_API_BASE_URL=https://stratadb-production.up.railway.app
```

### Backend Deployment — Railway

The C++ REST API is deployed on Railway using Docker.

**Backend URL:**

https://stratadb-production.up.railway.app

**Health endpoint:**

https://stratadb-production.up.railway.app/health

The backend runs inside a Docker container and listens on the configured Railway port.

### Container Configuration

The backend Docker image builds the C++ application using CMake and runs the server with `/data` as its database directory.

```dockerfile
CMD ["./build/stratadb_server", "/data"]
```

### Deployment Workflow

1. Source code is maintained in GitHub.
2. Railway builds and deploys the C++ backend.
3. Vercel builds and deploys the React frontend.
4. The frontend communicates with the backend through HTTPS REST APIs.
5. The backend stores database files in the mounted persistent volume.

---

## Data Persistence

StrataDB stores database files in the configured database directory.

In the deployed environment, the database directory is:

```text
/data
```

Railway provides a persistent volume mounted at `/data`.

This ensures database files remain available when the backend container is restarted or replaced during a deployment.

### Persistence Verification

The deployed application has been tested for:

- Inserting records.
- Retrieving stored records.
- Restarting the backend service.
- Retrieving records after a backend restart.
- Triggering a new backend deployment.
- Retrieving records after the new deployment.

The records remained available after both backend restarts and new deployments.

**Important:** Persistent volume storage does not replace a backup strategy. Deleting the volume or experiencing storage failure can result in data loss.

StrataDB also does not currently claim complete operating-system-level durability guarantees using explicit `fsync` or Windows `FlushFileBuffers` calls.

---

## Testing

StrataDB includes a GoogleTest-based test suite covering its core storage-engine components and regression scenarios.

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
- WAL truncated-payload handling.
- Compaction regression scenarios.

### Run the Complete Test Suite

From the project root:

```powershell
ctest --test-dir build --output-on-failure
```

**Latest verified development result: 109 out of 109 tests passing.**

### Automatic Flush Tests

The project also includes standalone tests for automatic flushing and recovery:

```powershell
.\build\auto_flush_manual_test.exe
```

```powershell
.\build\auto_flush_recovery_test.exe
```

These tests exercise threshold-triggered flushing and persistence after reopening the database.

### REST API Integration Verification

The HTTP API has been tested for:

- Health checks.
- PUT and update operations.
- GET operations.
- Range scans.
- Statistics.
- DELETE operations.
- Missing-key responses.
- Automatic flushing through the configured threshold.

Restart persistence was also verified by writing a key, stopping the server, restarting it, and retrieving the stored value.

---

## Benchmarking

StrataDB includes a standalone benchmark executable that measures the performance of selected storage-engine operations.

The benchmark covers:

| Operation | Description |
|---|---|
| PUT | Measures individual writes to the MemTable |
| GET | Measures reads from a database prepared with flushed SSTable data |
| Range Scan | Measures ordered range scans |
| Compaction | Measures SSTable compaction |

### Build the Benchmark

The benchmark is included in the CMake build:

```powershell
cmake --build build
```

### Run the Benchmark

Run with the default dataset size of 1,000 records:

```powershell
.\build\stratadb_benchmark.exe
```

Specify a dataset size:

```powershell
.\build\stratadb_benchmark.exe 5000
```

Specify both dataset size and output CSV path:

```powershell
.\build\stratadb_benchmark.exe 1000 .\benchmark-results.csv
```

By default, results are written to:

```text
benchmarks/results/stratadb_benchmark.csv
```

The generated CSV includes:

- Dataset size.
- Number of operations.
- Total execution time.
- Throughput.
- Average latency.
- p50 latency.
- p95 latency.
- p99 latency.
- Returned record count where applicable.

### Initial Local Benchmark Results

The following measurements were collected during local development.

| Operation | 500 records | 1,000 records | 5,000 records |
|---|---:|---:|---:|
| PUT throughput (ops/sec) | 2,011.74 | 1,230.51 | 346.69 |
| GET throughput (ops/sec) | 399,265.35 | 1,120,071.69 | 448,153.16 |
| Range scan time (ms, 3 scans) | 5.936 | 9.851 | 27.221 |
| Compaction time (ms) | 16.632 | 23.436 | 50.198 |
| Verified records after compaction | 500 | 1,000 | 5,000 |

**Benchmark notes:**

- These are initial local measurements, not universal performance guarantees.
- GET measurements use a database prepared with flushed SSTable data and may benefit from operating-system caching.
- Range scans use three repetitions per dataset; percentile values from such a small sample should not be treated as statistically representative.
- Results depend on hardware, compiler settings, operating system, dataset size, and runtime conditions.
- These benchmarks are intended to provide a repeatable baseline for future optimization.

---

## Current Limitations

StrataDB is an educational and portfolio-oriented storage-engine project. It demonstrates important database concepts, but it should not yet be treated as a production database.

Current engineering limitations include:

- Stronger operating-system-level durability guarantees.
- More extensive crash and recovery testing.
- Compaction crash consistency and orphan-file cleanup.
- Manifest and SSTable consistency validation.
- Concurrency control and stress testing.
- More comprehensive REST API error handling.
- Authentication and TLS.
- Restricted CORS before public exposure.
- Broader benchmark comparisons against established storage engines.

The HTTP server currently serializes requests using a mutex, and its development CORS configuration should not be considered production-ready.

In particular, the project does not currently claim a complete operating-system-level durability strategy using explicit `fsync` or Windows `FlushFileBuffers` calls.

---

## Author

**Amritanshu Singh**

- GitHub: [Amrittt22](https://github.com/Amrittt22)
- Project Repository: [StrataDB](https://github.com/Amrittt22/StrataDB)
- Live Dashboard: [StrataDB](https://frontend-amrit7.vercel.app/)

---

<p align="center">
  <strong>StrataDB</strong>
  <br/>
  Exploring how storage engines work, one component at a time.
</p>