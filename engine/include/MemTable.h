#ifndef STRATADB_MEMTABLE_H
#define STRATADB_MEMTABLE_H

#include <string>
#include <cstddef>
#include "SkipList.h"

class MemTable {
private:
    SkipList storage;
    std::size_t entryCount;

public:
    MemTable();

    void put(const std::string& key,
             const std::string& value);

    bool get(const std::string& key,
             std::string& value) const;

    bool remove(const std::string& key);

    std::size_t size() const;

    bool empty() const;
};

#endif
