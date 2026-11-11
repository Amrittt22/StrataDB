# LSM-tree

StrataDB follows the core LSM pattern: absorb writes in memory and write immutable sorted files in batches. Its levels are represented by one active MemTable and a vector of SSTables managed newest-first; it does not implement a multi-level leveled/size-tiered policy.

The visible value for a key is the record with the greatest sequence number across those sources. Tombstones participate in that ordering and suppress older values. See the [write path](../architecture/write-path.md), [read path](../architecture/read-path.md), and [compaction](compaction.md).
