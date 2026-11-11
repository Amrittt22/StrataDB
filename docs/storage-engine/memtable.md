# MemTable and SkipList

`MemTable` stores live values in `SkipList`, tracks per-key sequence numbers, and maintains a tombstone set. `SkipList` provides ordered insertion, search, removal, and enumeration with up to 16 randomized levels and probability `0.5`.

`getSnapshot()` emits both live `SSTableEntry` values and tombstones, preserving sequence numbers for flushing. `clearAfterCheckpoint()` removes only entries covered by the manifest checkpoint, preventing newer mutations from being discarded.

The MemTable is not independently persisted: the WAL is the recovery source until a successful flush/checkpoint.
