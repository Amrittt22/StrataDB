#ifndef STRATADB_DATABASE_H
#define STRATADB_DATABASE_H

#include "MemTable.h"
#include "Manifest.h"
#include "SSTableManager.h"

#include <string>
#include <utility>
#include <vector>

class Database {
private:
    Manifest manifest;
    MemTable memTable;
    SSTableManager sstableManager;

    bool opened;
    std::size_t memTableFlushThreshold;
    bool lastAutoFlushSuccessful;

    bool maybeAutoFlush();

    static std::string prepareWalPath(const std::string& dataDirectory);
    static std::string prepareSSTablePath(const std::string& dataDirectory);
    static std::string prepareManifestPath(
        const std::string& dataDirectory
    );
    static SequenceNumber loadCheckpoint(
        const std::string& dataDirectory
    );
    std::vector<std::pair<std::string, std::string>>
    readVisibleRecords(
        const std::string* startKey,
        const std::string* endKey
    ) const;

public:
    explicit Database(
        const std::string& dataDirectory,
        std::size_t flushThreshold = 1000
    );

    bool open();

    bool put(
        const std::string& key,
        const std::string& value
    );

    bool get(
        const std::string& key,
        std::string& value
    ) const;

    // Returns true if the key existed before deletion.
    bool remove(const std::string& key);

    bool isOpen() const;
    bool flushMemTable();

    std::vector<std::pair<std::string, std::string>> scan(
        const std::string& startKey,
        const std::string& endKey
    ) const;

    std::vector<std::pair<std::string, std::string>> list() const;

    bool compact();

    bool wasLastAutoFlushSuccessful() const;

    std::size_t memTableSize() const;
    std::size_t sstableCount() const;
};

#endif