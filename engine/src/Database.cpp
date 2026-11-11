#include "../include/Database.h"

#include <filesystem>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

std::string Database::prepareWalPath(
    const std::string& dataDirectory
) {
    fs::create_directories(dataDirectory);

    return (
        fs::path(dataDirectory) / "wal.log"
    ).string();
}

std::string Database::prepareSSTablePath(
    const std::string& dataDirectory
) {
    return (
        fs::path(dataDirectory) / "sstables"
    ).string();
}

std::string Database::prepareManifestPath(
    const std::string& dataDirectory
) {
    fs::create_directories(dataDirectory);

    return (
        fs::path(dataDirectory) / "MANIFEST"
    ).string();
}

SequenceNumber Database::loadCheckpoint(
    const std::string& dataDirectory
) {
    Manifest storedManifest(prepareManifestPath(dataDirectory));

    if (!storedManifest.load()) {
        throw std::runtime_error(
            "StrataDB: manifest is corrupted"
        );
    }

    return storedManifest.getCheckpointSequence();
}

Database::Database(
    const std::string& dataDirectory,
    std::size_t flushThreshold
)
    : manifest(prepareManifestPath(dataDirectory)),
      memTable(
          prepareWalPath(dataDirectory),
          loadCheckpoint(dataDirectory)
      ),
      sstableManager(prepareSSTablePath(dataDirectory)),
      opened(false),
      memTableFlushThreshold(flushThreshold),
      lastAutoFlushSuccessful(true) {
}

bool Database::open() {
    if (opened) {
        return true;
    }

    if (!manifest.load() ||
        !sstableManager.loadExisting()) {
        return false;
    }

    opened = true;
    return true;
}

bool Database::put(
    const std::string& key,
    const std::string& value
) {
    if (!opened) {
        return false;
    }

    if (!memTable.put(key, value)) {
        return false;
    }

    maybeAutoFlush();
    return true;
}

bool Database::get(
    const std::string& key,
    std::string& value
) const {
    if (!opened) {
        return false;
    }

    // 1. Check the MemTable first.
    if (memTable.get(key, value)) {
        return true;
    }

    // 2. A MemTable tombstone must hide older SSTable values.
    if (memTable.hasTombstone(key)) {
        return false;
    }

    // 3. Search SSTables from newest to oldest.
    LookupResult result = sstableManager.lookup(key, value);

    return result == LookupResult::VALUE_FOUND;
}

bool Database::remove(const std::string& key) {
    if (!opened) {
        return false;
    }

    std::string existingValue;
    const bool existed = get(key, existingValue);
    bool tombstoneRecorded = false;

    if (!memTable.remove(key, tombstoneRecorded)) {
        return false;
    }

    maybeAutoFlush();
    return existed;
}

bool Database::maybeAutoFlush() {
    if (!opened || memTableFlushThreshold == 0) {
        return true;
    }

    const std::size_t currentEntries =
        memTable.getSnapshot().size();

    if (currentEntries < memTableFlushThreshold) {
        return true;
    }

    lastAutoFlushSuccessful = flushMemTable();
    return lastAutoFlushSuccessful;
}

bool Database::isOpen() const {
    return opened;
}

std::size_t Database::memTableSize() const {
    return memTable.size();
}

std::size_t Database::sstableCount() const {
    return sstableManager.count();
}

bool Database::flushMemTable() {
    if (!opened) {
        return false;
    }

    // Capture live entries and tombstones.
    std::vector<SSTableEntry> snapshot = memTable.getSnapshot();

    // Nothing to flush.
    if (snapshot.empty()) {
        return true;
    }

    SequenceNumber checkpointSequence = 0;

    for (const auto& entry : snapshot) {
        checkpointSequence =
            std::max(checkpointSequence, entry.sequence);
    }

    if (checkpointSequence == 0) {
        return false;
    }

    if (!sstableManager.addSSTable(snapshot)) {
        return false;
    }

    if (!manifest.save(checkpointSequence)) {
        return false;
    }

    return memTable.clearAfterCheckpoint(checkpointSequence);
}

std::vector<std::pair<std::string, std::string>>
Database::readVisibleRecords(
    const std::string* startKey,
    const std::string* endKey
) const {
    std::vector<std::pair<std::string, std::string>> result;

    if (!opened ||
        (startKey && endKey && *startKey > *endKey)) {
        return result;
    }

    std::map<std::string, SSTableEntry> visible;
    const auto diskEntries =
        sstableManager.getEntriesNewestFirst();

    for (const auto& entry : diskEntries) {
        if ((startKey && entry.key < *startKey) ||
            (endKey && entry.key > *endKey)) {
            continue;
        }

        auto it = visible.find(entry.key);

        if (it == visible.end() ||
            entry.sequence > it->second.sequence) {
            visible[entry.key] = entry;
        }
    }

    for (const auto& entry : memTable.getSnapshot()) {
        if ((!startKey || entry.key >= *startKey) &&
            (!endKey || entry.key <= *endKey)) {
            auto it = visible.find(entry.key);

            if (it == visible.end() ||
                entry.sequence >= it->second.sequence) {
                visible[entry.key] = entry;
            }
        }
    }

    for (const auto& item : visible) {
        if (!item.second.isTombstone) {
            result.emplace_back(item.first, item.second.value);
        }
    }

    return result;
}

std::vector<std::pair<std::string, std::string>>
Database::scan(
    const std::string& startKey,
    const std::string& endKey
) const {
    return readVisibleRecords(&startKey, &endKey);
}

std::vector<std::pair<std::string, std::string>>
Database::list() const {
    return readVisibleRecords(nullptr, nullptr);
}

bool Database::compact() {
    if (!opened || !flushMemTable()) {
        return false;
    }

    return sstableManager.compactAll();
}

bool Database::wasLastAutoFlushSuccessful() const {
    return lastAutoFlushSuccessful;
}