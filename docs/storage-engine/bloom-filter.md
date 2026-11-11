# Bloom filters

Each loaded SSTable owns a Bloom filter sized from expected entries with a default false-positive target of 1%. It uses two hash functions and a bit vector. `mightContain(key) == false` safely skips the table; `true` requires the normal sorted lookup because false positives are expected.

Bloom filters optimize point lookup only. Range scans and `/records` enumerate loaded entries and merge them by sequence.
