# Tests and benchmarks

## C++ tests

The root `CMakeLists.txt` builds `stratadb_tests` from the engine test files and registers GoogleTest cases with CTest. The latest verified run was:

```text
100% tests passed out of 110
```

Coverage includes:

- SkipList insertion, lookup, ordering, and deletion.
- MemTable updates, sequence numbers, tombstones, snapshots, and checkpoint clearing.
- WAL encoding, replay, invalid records, and incomplete-tail recovery.
- SSTable serialization, validation, Bloom-filter lookup, and tombstones.
- Database CRUD, recovery, flushes, range scans, MemTable/SSTable precedence, `/records`-equivalent list merging, and compaction.
- Statistics, logger, automatic flush, and project-level smoke tests.

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Use temporary data directories for destructive or recovery tests. Do not use Railway `/data`.

## API and dashboard verification

The server has been exercised locally with an isolated temporary database for health, PUT, DELETE, `/scan`, and `/records` behavior. The backend list merge test verifies latest values and tombstone omission. Browser interaction checks for search, pagination, details, and copy are frontend behavior checks rather than a dedicated automated Playwright suite.

Frontend checks:

```powershell
cd frontend
npm run build
npm run lint
```

## Benchmark

`engine/benchmarks/benchmark.cpp` creates temporary databases and verifies each result before writing CSV. The checked-in `benchmarks/results/stratadb_benchmark.csv` is a 1,000-record local run:

| Operation | Total time | Throughput | Average | p50 | p95 | p99 |
|---|---:|---:|---:|---:|---:|---:|
| put_memtable | 736.354 ms | 1,358.043 ops/s | 736.354 us | 639.500 us | 1,715.900 us | 2,267.300 us |
| get_sstable | 3.344 ms | 299,025.178 ops/s | 3.344 us | 2.900 us | 5.000 us | 5.600 us |
| range_scan | 6.236 ms | 481.108 ops/s | 2,078.533 us | 1,943.500 us | 1,943.500 us | 1,943.500 us |
| compaction | 29.659 ms | 33.717 ops/s | 29,658.800 us | 29,658.800 us | 29,658.800 us | 29,658.800 us |

These are local measurements, not production performance claims.
