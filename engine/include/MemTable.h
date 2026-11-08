#ifndef STRATADB_MEMTABLE_H
#define STRATADB_MEMTABLE_H

#include <string>
#include <cstddef>
#include <vector>
#include <utility>

#include "SkipList.h"
#include "WAL.h"

class MemTable {
private:
    SkipList storage;
    std::size_t entryCount;
    WAL wal;

    // Internal operations that do not write to WAL.
    bool putInternal(
        const std::string& key,
        const std::string& value
    );

    bool removeInternal(const std::string& key);

    friend class WAL;

public:
    explicit MemTable(
        const std::string& walPath = "stratadb.wal"
    );

    MemTable(const MemTable&) = delete;
    MemTable& operator=(const MemTable&) = delete;

    bool put(
        const std::string& key,
        const std::string& value
    );

    bool get(
        const std::string& key,
        std::string& value
    ) const;

    bool remove(const std::string& key);

    std::size_t size() const;

    bool empty() const;

    std::vector<std::pair<std::string, std::string>> getAllEntries() const;
};

#endif