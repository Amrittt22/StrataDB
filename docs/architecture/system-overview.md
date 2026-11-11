# System overview

StrataDB has three layers: a React/Vite client, a cpp-httplib server, and the storage engine.

```mermaid
flowchart TB
    Browser[Browser] --> Frontend[React + TypeScript dashboard]
    Frontend -->|HTTP JSON/raw text| Server[cpp-httplib server]
    Server --> Database[Database coordinator]
    Database --> MemTable[MemTable / SkipList]
    Database --> WAL[WAL]
    Database --> Tables[SSTableManager]
    Tables --> SSTables[Immutable SSTables]
    Tables --> Bloom[Bloom filters]
    Database --> Manifest[MANIFEST checkpoint]
    SSTables --> Volume[Database directory]
    WAL --> Volume
    Manifest --> Volume
```

`Database` owns one `MemTable`, `Manifest`, and `SSTableManager`. The server shares one `Database` instance across routes and protects operations with a global mutex. SSTable files are stored under `<data-directory>/sstables`; the WAL and manifest are directly under the data directory.

The deployed backend uses `/data`. Local development should use a separate directory such as `./stratadb_data`.
