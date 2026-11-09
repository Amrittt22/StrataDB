#ifndef STRATADB_MANIFEST_H
#define STRATADB_MANIFEST_H

#include "Record.h"

#include <string>

class Manifest {
private:
    std::string filePath;
    SequenceNumber checkpointSequence;

public:
    explicit Manifest(const std::string& path);

    bool load();

    bool save(SequenceNumber sequence);

    SequenceNumber getCheckpointSequence() const;
};

#endif