#ifndef STRATADB_SSTABLE_MANAGER_H
#define STRATADB_SSTABLE_MANAGER_H

#include "SSTable.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class SSTableManager {
private:
    struct ManagedTable {
        std::uint64_t id;
        std::string filePath;
        std::unique_ptr<SSTable> table;
    };

    std::string directory;
    std::vector<ManagedTable> tables; // Newest first

    std::uint64_t nextId;
    bool loaded;

    static bool parseTableId(
        const std::string& filename,
        std::uint64_t& id
    );

    std::string makeFilePath(std::uint64_t id) const;

public:
    explicit SSTableManager(const std::string& directory);

    // Discover and load all existing SSTables.
    bool loadExisting();

    // Persist a new SSTable and add it to the manager.
    bool addSSTable(
        const std::vector<SSTableEntry>& entries,
        std::uint64_t* createdId = nullptr
    );

    // Search newest to oldest.
    LookupResult lookup(
        const std::string& key,
        std::string& value
    ) const;

    std::size_t count() const;

    std::vector<SSTableEntry> getEntriesNewestFirst() const;

    bool compactAll();

    // Returns IDs from newest to oldest.
    std::vector<std::uint64_t> getSSTableIds() const;

    bool isLoaded() const;
};

#endif