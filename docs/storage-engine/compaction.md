# Compaction

Compaction is invoked by `Database::compact()`. It flushes pending MemTable data, then merges every loaded SSTable into one new table. A `std::map` groups entries by key and keeps the entry with the greatest sequence number, including a tombstone. The new table is added before old files are removed.

This preserves current visibility and removes obsolete versions, but there is no automatic scheduling, crash journal for the multi-file compaction sequence, or multi-level compaction strategy.
