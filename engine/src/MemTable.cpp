#include "MemTable.h"

#include <limits>
#include <stdexcept>
#include <utility>

MemTable::MemTable(
    const std::string& walPath,
    SequenceNumber checkpointSequence
)
    : entryCount(0),
      nextSequence(checkpointSequence + 1),
      wal(walPath) {

    if (checkpointSequence ==
        std::numeric_limits<SequenceNumber>::max()) {
        throw std::runtime_error(
            "StrataDB checkpoint sequence exhausted"
        );
    }

    if (!wal.replay(*this, checkpointSequence)) {
        throw std::runtime_error(
            "StrataDB startup failed: WAL recovery unsuccessful"
        );
    }
}

bool MemTable::put(
    const std::string& key,
    const std::string& value
) {
    if (nextSequence == 0 ||
        nextSequence == std::numeric_limits<SequenceNumber>::max()) {
        return false;
    }

    const SequenceNumber sequence = nextSequence;

    if (!wal.appendPut(sequence, key, value)) {
        return false;
    }

    return putInternal(key, value, sequence);
}

bool MemTable::putInternal(
    const std::string& key,
    const std::string& value,
    SequenceNumber sequence
) {
    if (sequence == 0 ||
        sequence == std::numeric_limits<SequenceNumber>::max()) {
        return false;
    }

    std::string existingValue;

    if (!storage.search(key, existingValue)) {
        ++entryCount;
    }

    storage.insert(key, value);

    tombstones.erase(key);

    sequences[key] = sequence;

    if (sequence >= nextSequence) {
        nextSequence = sequence + 1;
    }

    return true;
}

bool MemTable::get(
    const std::string& key,
    std::string& value
) const {
    if (tombstones.find(key) != tombstones.end()) {
        return false;
    }

    return storage.search(key, value);
}
bool MemTable::remove(const std::string& key) {
    bool existed = false;

    if (!remove(key, existed)) {
        return false;
    }

    return existed;
}

bool MemTable::remove(
    const std::string& key,
    bool& existed
) {

    if (nextSequence == 0 ||
        nextSequence == std::numeric_limits<SequenceNumber>::max()) {
        return false;
    }

    std::string existingValue;
    existed = storage.search(key, existingValue);

    const SequenceNumber sequence = nextSequence;

    if (!wal.appendDelete(sequence, key)) {
        return false;
    }

    return removeInternal(key, sequence);
}

bool MemTable::removeInternal(
    const std::string& key,
    SequenceNumber sequence
) {
    if (sequence == 0 ||
        sequence == std::numeric_limits<SequenceNumber>::max()) {
        return false;
    }

    std::string existingValue;

    if (storage.search(key, existingValue)) {
        storage.remove(key);
        --entryCount;
    }

    tombstones.insert(key);
    sequences[key] = sequence;

    if (sequence >= nextSequence) {
        nextSequence = sequence + 1;
    }

    return true;
}

bool MemTable::hasTombstone(const std::string& key) const {
    return tombstones.find(key) != tombstones.end();
}

std::size_t MemTable::size() const {
    return entryCount;
}

bool MemTable::empty() const {
    return entryCount == 0;
}

std::vector<std::pair<std::string, std::string>>
MemTable::getAllEntries() const {
    return storage.getAllEntries();
}

std::vector<std::string>
MemTable::getAllTombstones() const {
    return std::vector<std::string>(
        tombstones.begin(),
        tombstones.end()
    );
}

std::vector<SSTableEntry> MemTable::getSnapshot() const {

    std::vector<SSTableEntry> snapshot;

    for (const auto& entry : storage.getAllEntries()) {

        auto it = sequences.find(entry.first);

        if (it == sequences.end()) {
            continue;
        }

        snapshot.push_back({
            entry.first,
            entry.second,
            false,
            it->second
        });
    }

    for (const auto& key : tombstones) {

        auto it = sequences.find(key);

        if (it == sequences.end()) {
            continue;
        }

        snapshot.push_back({
            key,
            "",
            true,
            it->second
        });
    }

    return snapshot;
}

bool MemTable::clearAfterCheckpoint(
    SequenceNumber checkpointSequence
) {
    if (checkpointSequence >= nextSequence ||
        checkpointSequence ==
            std::numeric_limits<SequenceNumber>::max()) {
        return false;
    }

    const auto entries = storage.getAllEntries();

    for (const auto& entry : entries) {
        storage.remove(entry.first);
    }

    tombstones.clear();
    sequences.clear();
    entryCount = 0;

    const bool walReset = wal.reset();
    nextSequence = checkpointSequence + 1;

    return walReset;
}

SequenceNumber MemTable::getLastSequence() const {
    return nextSequence - 1;
}

SequenceNumber MemTable::getNextSequence() const {
    return nextSequence;
}

std::vector<Record> MemTable::getAllRecords() const {

    std::vector<Record> records;

    for (const auto& entry : storage.getAllEntries()) {

        auto it = sequences.find(entry.first);

        if (it != sequences.end()) {
            records.emplace_back(
                entry.first,
                entry.second,
                it->second,
                false
            );
        }
    }

    for (const auto& key : tombstones) {

        auto it = sequences.find(key);

        if (it != sequences.end()) {
            records.emplace_back(
                key,
                "",
                it->second,
                true
            );
        }
    }

    return records;
}