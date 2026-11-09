#ifndef STRATADB_RECORD_H
#define STRATADB_RECORD_H

#include <cstdint>
#include <string>

using SequenceNumber = std::uint64_t;

struct Record {
    std::string key;
    std::string value;
    SequenceNumber sequence;
    bool isTombstone;

    Record(
        const std::string& key,
        const std::string& value,
        SequenceNumber sequence,
        bool isTombstone = false
    )
        : key(key),
          value(value),
          sequence(sequence),
          isTombstone(isTombstone) {}
};

#endif