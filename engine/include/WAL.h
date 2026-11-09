#ifndef STRATADB_WAL_H
#define STRATADB_WAL_H

#include <cstdint>
#include <string>

using SequenceNumber = std::uint64_t;

class MemTable;

class WAL
{
private:
    std::string filePath;
    SequenceNumber legacyNextSequence = 1;

    bool appendRecord(
        std::uint8_t operation,
        SequenceNumber sequence,
        const std::string &key,
        const std::string &value);

public:
    explicit WAL(const std::string &path);

    // Sequence-aware API used by MemTable.
    bool appendPut(
        SequenceNumber sequence,
        const std::string &key,
        const std::string &value);

    bool appendDelete(
        SequenceNumber sequence,
        const std::string &key);

    // Compatibility overloads for existing WAL tests.
    bool appendPut(
        const std::string &key,
        const std::string &value);

    bool appendDelete(const std::string &key);

    bool replay(
        MemTable &table,
        SequenceNumber checkpointSequence = 0) const;
    bool reset();
};

#endif