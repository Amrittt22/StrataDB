#include "MemTable.h"

MemTable::MemTable()
    : entryCount(0) {
}

void MemTable::put(const std::string& key,
                   const std::string& value) {

    std::string existingValue;

    // Increase count only for a new key.
    if (!storage.search(key, existingValue)) {
        entryCount++;
    }

    storage.insert(key, value);
}

bool MemTable::get(const std::string& key,
                   std::string& value) const {

    return storage.search(key, value);
}

bool MemTable::remove(const std::string& key) {

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
