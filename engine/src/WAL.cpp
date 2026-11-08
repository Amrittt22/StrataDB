#include <filesystem>
#include <system_error>
#include <cstdint>
#include "WAL.h"
#include "MemTable.h"

#include <fstream>
#include <limits>

namespace {
    constexpr std::uint8_t PUT_OPERATION = 1;
    constexpr std::uint8_t DELETE_OPERATION = 2;

    constexpr std::uint32_t MAX_RECORD_SIZE = 64 * 1024 * 1024;

    // Write a 32-bit integer in little-endian format.
    void writeUint32(std::ofstream& out, std::uint32_t value) {
        char bytes[4];

        bytes[0] = static_cast<char>(value);
        bytes[1] = static_cast<char>(value >> 8);
        bytes[2] = static_cast<char>(value >> 16);
        bytes[3] = static_cast<char>(value >> 24);

        out.write(bytes, 4);
    }

    // Read a 32-bit integer in little-endian format.
    bool readUint32(std::ifstream& in, std::uint32_t& value) {
        unsigned char bytes[4];

        if (!in.read(reinterpret_cast<char*>(bytes), 4)) {
            return false;
        }

        value =
            static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8) |
            (static_cast<std::uint32_t>(bytes[2]) << 16) |
            (static_cast<std::uint32_t>(bytes[3]) << 24);

        return true;
    }
}

WAL::WAL(const std::string& path)
    : filePath(path) {
}

bool WAL::appendRecord(
    std::uint8_t operation,
    const std::string& key,
    const std::string& value
) {
    if (key.size() > MAX_RECORD_SIZE ||
        value.size() > MAX_RECORD_SIZE) {
        return false;
    }

    if (key.size() > std::numeric_limits<std::uint32_t>::max() ||
        value.size() > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    std::ofstream out(filePath, std::ios::binary | std::ios::app);

    if (!out.is_open()) {
        return false;
    }

    out.put(static_cast<char>(operation));

    writeUint32(out, static_cast<std::uint32_t>(key.size()));
    writeUint32(out, static_cast<std::uint32_t>(value.size()));

    out.write(key.data(), static_cast<std::streamsize>(key.size()));
    out.write(value.data(), static_cast<std::streamsize>(value.size()));

    out.flush();

    if (!out) {
        return false;
    }

    out.close();

    return !out.fail();
}

bool WAL::appendPut(
    const std::string& key,
    const std::string& value
) {
    return appendRecord(PUT_OPERATION, key, value);
}

bool WAL::appendDelete(const std::string& key) {
    return appendRecord(DELETE_OPERATION, key, "");
}

bool WAL::replay(MemTable& table) const {

    std::ifstream in(filePath, std::ios::binary);

    if (!in.is_open()) {
        return true;
    }

    std::error_code ec;
    const std::uintmax_t fileSize =
        std::filesystem::file_size(filePath, ec);

    if (ec) {
        return false;
    }

    std::uintmax_t offset = 0;

    // Remove an incomplete final record while preserving
    // all previously completed records.
    auto truncateTail = [&](std::uintmax_t validSize) {
        in.close();

        std::error_code truncateError;

        std::filesystem::resize_file(
            filePath,
            validSize,
            truncateError
        );

        return !truncateError;
    };

    while (offset < fileSize) {

        const std::uintmax_t recordStart = offset;
        const std::uintmax_t remaining = fileSize - offset;

        // Every record requires:
        // 1 byte operation + 4 bytes key length
        // + 4 bytes value length.
        constexpr std::uintmax_t HEADER_SIZE = 9;

        if (remaining < HEADER_SIZE) {
            return truncateTail(recordStart);
        }

        char operationByte;

        if (!in.get(operationByte)) {
            return false;
        }

        std::uint32_t keyLength;
        std::uint32_t valueLength;

        if (!readUint32(in, keyLength) ||
            !readUint32(in, valueLength)) {
            return false;
        }

        offset += HEADER_SIZE;

        if (keyLength > MAX_RECORD_SIZE ||
            valueLength > MAX_RECORD_SIZE) {
            return false;
        }

        std::uint8_t operation =
            static_cast<std::uint8_t>(
                static_cast<unsigned char>(operationByte)
            );

        // Reject invalid operations.
        if (operation != PUT_OPERATION &&
            operation != DELETE_OPERATION) {
            return false;
        }

        // DELETE records must not contain a value.
        if (operation == DELETE_OPERATION &&
            valueLength != 0) {
            return false;
        }

        const std::uint64_t payloadSize =
            static_cast<std::uint64_t>(keyLength) +
            static_cast<std::uint64_t>(valueLength);

        // The final record is incomplete.
        if (fileSize - offset < payloadSize) {
            return truncateTail(recordStart);
        }

        std::string key(keyLength, '\0');
        std::string value(valueLength, '\0');

        if (!in.read(key.data(),
                     static_cast<std::streamsize>(keyLength))) {
            return false;
        }

        if (!in.read(value.data(),
                     static_cast<std::streamsize>(valueLength))) {
            return false;
        }

        // Apply directly to MemTable without writing
        // duplicate records into WAL.
        if (operation == PUT_OPERATION) {
            table.putInternal(key, value);
        }
        else {
            table.removeInternal(key);
        }

        offset += payloadSize;
    }

    return true;
}
