# Write path

`Database::put` delegates to `MemTable::put`, which assigns a sequence number and appends a sequence-aware PUT record to the WAL before updating the SkipList. The configured threshold counts the current MemTable snapshot; reaching it triggers `flushMemTable`.

```mermaid
flowchart TD
    P[PUT /kv/key] --> D[Database::put]
    D --> W[WAL appendPut(sequence, key, value)]
    W --> M[MemTable SkipList insert]
    M --> T{Snapshot >= threshold?}
    T -->|no| R[Return success]
    T -->|yes| S[Write sorted SSTable snapshot]
    S --> F[Advance MANIFEST checkpoint]
    F --> C[Clear checkpointed MemTable entries]
    C --> Z[Reset checkpointed WAL]
    Z --> R
```

Deletes follow the same route with `appendDelete` and a tombstone. `Database::remove` returns whether the key existed before recording the tombstone; the HTTP route returns 404 when it did not.
