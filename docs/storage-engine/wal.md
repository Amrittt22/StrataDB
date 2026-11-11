# Write-Ahead Log

The WAL is a binary append-only file, normally `<data-directory>/wal.log`. Each V2 record contains:

```text
operation (1 byte)
sequence (8 bytes, little-endian)
key length (4 bytes)
value length (4 bytes)
key bytes
value bytes
```

PUT is operation `1`; DELETE is operation `2` and must have an empty value. Keys and values are capped at 64 MiB by validation. Replay enforces positive, strictly increasing sequence numbers, skips records at or below the manifest checkpoint, and truncates an incomplete final record. Invalid operations, lengths, ordering, or payloads fail replay.

After a successful SSTable write and manifest save, checkpointed MemTable state is cleared and the WAL is reset by the MemTable checkpoint path. The implementation flushes the file stream but does not expose an `fsync` durability contract.
