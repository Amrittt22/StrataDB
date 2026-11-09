
#ifndef STRATADB_SSTABLE_H
#define STRATADB_SSTABLE_H

#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include "Record.h"
#include "BloomFilter.h"

struct SSTableEntry {
    std::string key;
    std::string value;
    bool isTombstone = false;
    SequenceNumber sequence = 0;
};

enum class LookupResult {
    NOT_FOUND,
    VALUE_FOUND,
    TOMBSTONE_FOUND
};

class SSTable {
private:
    static constexpr std::uint32_t MAX_FIELD_SIZE =
        64 * 1024 * 1024;

    static constexpr std::uint32_t MAX_ENTRY_COUNT =
        10000000;

    std::vector<SSTableEntry> entries;
    BloomFilter bloomFilter;
    bool loaded;

    static bool writeUint32(
        std::ofstream& out,
        std::uint32_t value
    );

    static bool readUint32(
        std::ifstream& in,
        std::uint32_t& value
    );

public:
    explicit SSTable(std::size_t expectedEntries = 1000);

    // Write entries including tombstones.
    static bool writeEntries(
        const std::string& filePath,
        const std::vector<SSTableEntry>& data
    );

    // Backward-compatible convenience overload for normal values.
    static bool write(
        const std::string& filePath,
        const std::vector<std::pair<std::string, std::string>>& data
    );

    bool load(const std::string& filePath);

    // Distinguishes missing keys from tombstones.
    LookupResult lookup(
        const std::string& key,
        std::string& value
    ) const;

    // Convenience lookup: returns true only for a live value.
    bool get(
        const std::string& key,
        std::string& value
    ) const;

    bool mightContain(const std::string& key) const;

    std::size_t size() const;

    const std::vector<SSTableEntry>& getEntries() const;

    bool isLoaded() const;
};

#endif
