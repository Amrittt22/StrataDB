#ifndef STRATADB_SSTABLE_H
#define STRATADB_SSTABLE_H

#include <cstdint>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "BloomFilter.h"

class SSTable {
private:
    static constexpr std::uint32_t MAX_FIELD_SIZE = 64 * 1024 * 1024;

    std::vector<std::pair<std::string, std::string>> entries;
    BloomFilter bloomFilter;
    bool loaded;

    static bool writeUint32(std::ofstream& out, std::uint32_t value);
    static bool readUint32(std::ifstream& in, std::uint32_t& value);

public:
    explicit SSTable(std::size_t expectedEntries = 1000);

    static bool write(
        const std::string& filePath,
        const std::vector<std::pair<std::string, std::string>>& data
    );

    bool load(const std::string& filePath);
    bool get(const std::string& key, std::string& value) const;
    bool mightContain(const std::string& key) const;
    std::size_t size() const;
    bool isLoaded() const;
};

#endif
