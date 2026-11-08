#ifndef STRATADB_WAL_H
#define STRATADB_WAL_H

#include <string>
#include <cstdint>

class MemTable;

class WAL {
private:
    std::string filePath;

    bool appendRecord(
        std::uint8_t operation,
        const std::string& key,
        const std::string& value
    );

public:
    explicit WAL(const std::string& path);

    bool appendPut(
        const std::string& key,
        const std::string& value
    );

    bool appendDelete(const std::string& key);

    bool replay(MemTable& table) const;
};

#endif