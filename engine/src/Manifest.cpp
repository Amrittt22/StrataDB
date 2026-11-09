#include "Manifest.h"

#include <cstdio>
#include <fstream>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr char MAGIC[8] = {
    'S', 'T', 'R', 'A', 'T', 'A', 'M', '1'
};

void writeUint64(std::ofstream& out, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
    }
}

bool readUint64(std::ifstream& in, std::uint64_t& value) {
    value = 0;

    for (int i = 0; i < 8; ++i) {
        char byte;

        if (!in.get(byte)) {
            return false;
        }

        value |=
            static_cast<std::uint64_t>(
                static_cast<unsigned char>(byte)
            ) << (i * 8);
    }

    return true;
}

bool replaceFile(
    const std::string& temporary,
    const std::string& destination
) {
#ifdef _WIN32
    return MoveFileExA(
        temporary.c_str(),
        destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
    ) != 0;
#else
    return std::rename(
        temporary.c_str(),
        destination.c_str()
    ) == 0;
#endif
}

}

Manifest::Manifest(const std::string& path)
    : filePath(path),
      checkpointSequence(0) {
}

bool Manifest::load() {

    std::ifstream in(filePath, std::ios::binary);

    // A missing manifest means a new database.
    if (!in.is_open()) {
        checkpointSequence = 0;
        return true;
    }

    char magic[8];

    if (!in.read(magic, sizeof(magic))) {
        return false;
    }

    for (int i = 0; i < 8; ++i) {
        if (magic[i] != MAGIC[i]) {
            return false;
        }
    }

    SequenceNumber sequence;

    if (!readUint64(in, sequence)) {
        return false;
    }

    // Reject trailing or malformed data.
    if (in.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    checkpointSequence = sequence;

    return true;
}

bool Manifest::save(SequenceNumber sequence) {

    if (sequence < checkpointSequence) {
        return false;
    }

    const std::string temporaryPath = filePath + ".tmp";

    {
        std::ofstream out(
            temporaryPath,
            std::ios::binary | std::ios::trunc
        );

        if (!out.is_open()) {
            return false;
        }

        out.write(MAGIC, sizeof(MAGIC));
        writeUint64(out, sequence);

        out.flush();

        if (!out.good()) {
            return false;
        }

        out.close();

        if (out.fail()) {
            return false;
        }
    }

    if (!replaceFile(temporaryPath, filePath)) {
        std::remove(temporaryPath.c_str());
        return false;
    }

    checkpointSequence = sequence;

    return true;
}

SequenceNumber Manifest::getCheckpointSequence() const {
    return checkpointSequence;
}