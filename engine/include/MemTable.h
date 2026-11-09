
#ifndef STRATADB_MEMTABLE_H
#define STRATADB_MEMTABLE_H

#include <string>
#include <cstddef>
#include <vector>
#include <utility>
#include "SSTable.h"
#include <unordered_map>
#include <unordered_set>
#include "Record.h"
#include "SkipList.h"
#include "WAL.h"

class MemTable
{
private:
    SkipList storage;
    std::size_t entryCount;
    SequenceNumber nextSequence;
    std::unordered_map<std::string, SequenceNumber> sequences;

    // Keys deleted from this MemTable.
    // These markers must survive until they are safely flushed.
    std::unordered_set<std::string> tombstones;

    WAL wal;

   bool putInternal(
    const std::string& key,
    const std::string& value,
    SequenceNumber sequence
);

bool removeInternal(
    const std::string& key,
    SequenceNumber sequence
);

    friend class WAL;

public:
    explicit MemTable(
        const std::string& walPath = "stratadb.wal",
        SequenceNumber checkpointSequence = 0);

    MemTable(const MemTable &) = delete;
    MemTable &operator=(const MemTable &) = delete;

    bool put(
        const std::string &key,
        const std::string &value);

    bool get(
        const std::string &key,
        std::string &value) const;

    bool remove(const std::string &key);
    bool remove(const std::string& key, bool& existed);

    bool hasTombstone(const std::string &key) const;

    std::size_t size() const;

    bool empty() const;

    // Returns live key-value entries.
    std::vector<std::pair<std::string, std::string>>
    getAllEntries() const;

    // Returns deleted keys that must be persisted as tombstones.
    std::vector<std::string> getAllTombstones() const;

    std::vector<SSTableEntry> getSnapshot() const;
    bool clearAfterCheckpoint(SequenceNumber checkpointSequence);
    SequenceNumber getLastSequence() const;
    SequenceNumber getNextSequence() const;
    std::vector<Record> getAllRecords() const;
};

#endif
