# StrataDB documentation

This documentation describes the implemented C++17 engine, REST server, React dashboard, tests, benchmark, and deployment.

## Primary textbook

[`StrataDB_Textbook.pdf`](StrataDB_Textbook.pdf) is the existing primary comprehensive textbook. It is preserved in place as a binary ReportLab PDF because this repository does not contain its editable source and no PDF editing tool is part of the project toolchain. The Markdown chapters below provide the verified final-implementation reference for the completed Data Explorer, `/records` API, deployment configuration, and 110-test result without replacing the textbook.

## Architecture

- [System overview](architecture/system-overview.md)
- [Write path](architecture/write-path.md)
- [Read path](architecture/read-path.md)
- [Recovery and compaction](architecture/recovery-and-compaction.md)

## Storage engine

- [LSM tree](storage-engine/lsm-tree.md)
- [MemTable and SkipList](storage-engine/memtable.md)
- [WAL](storage-engine/wal.md)
- [SSTables](storage-engine/sstable.md)
- [Bloom filters](storage-engine/bloom-filter.md)
- [Compaction](storage-engine/compaction.md)

## Interfaces and operations

- [REST API](backend/rest-api.md)
- [Dashboard](frontend/dashboard.md)
- [Tests and benchmarks](testing/tests-and-benchmarks.md)
- [Deployment guide](deployment/deployment-guide.md)

The [StrataDB textbook](StrataDB_Textbook.pdf) is a supplementary project document.
