# SSTables

SSTables are immutable files named `sstable_<id>.sst`. The current format uses the magic marker `STRATAS3`, an entry count, and sorted entries containing key length, value length, entry type, sequence number, key bytes, and value bytes. Values and keys are capped at 64 MiB and tables at 10,000,000 entries.

Writes go to a temporary file, are loaded for validation, and are renamed into the final filename. On startup, `SSTableManager` scans the directory, parses numeric IDs, loads valid tables, and sorts them by ID descending. Lookup uses a Bloom filter followed by `lower_bound` over sorted entries.

Tombstones are stored as entries with an empty value and are retained until compaction's newest-version merge.
