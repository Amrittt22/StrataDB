#include "MemTable.h"

#include <stdexcept>

MemTable::MemTable(const std::string& walPath)
    : entryCount(0), wal(walPath) {

    if (!wal.replay(*this)) {
        throw std::runtime_error(
            "StrataDB startup failed: WAL recovery unsuccessful"
        );
    }
}

bool MemTable::put(
    const std::string& key,
    const std::string& value
) {
    // First persist the operation in WAL.
    if (!wal.appendPut(key, value)) {
        return false;
    }

    // Only update memory if WAL append succeeds.
    return putInternal(key, value);
}

bool MemTable::putInternal(
    const std::string& key,
    const std::string& value
) {
    std::string existingValue;

    if (!storage.search(key, existingValue)) {
        entryCount++;
    }

    storage.insert(key, value);

    return true;
}

bool MemTable::get(
    const std::string& key,
    std::string& value
) const {
    return storage.search(key, value);
}

bool MemTable::remove(const std::string& key) {

    // Log the deletion before changing memory.
    if (!wal.appendDelete(key)) {
        return false;
    }

    return removeInternal(key);
}

bool MemTable::removeInternal(const std::string& key) {

    if (!storage.remove(key)) {
        return false;
    }

    entryCount--;

    return true;
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