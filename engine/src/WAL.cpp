#include "WAL.h"
#include "MemTable.h"

#include <filesystem>
#include <fstream>
#include <limits>

namespace
{

    constexpr std::uint8_t PUT_OPERATION = 1;
    constexpr std::uint8_t DELETE_OPERATION = 2;

    constexpr std::uint32_t MAX_RECORD_SIZE =
        64 * 1024 * 1024;

    // V2 record layout:
    // operation: 1 byte
    // sequence:  8 bytes
    // key size:  4 bytes
    // value size: 4 bytes
    // key bytes
    // value bytes

    constexpr std::uint64_t HEADER_SIZE = 17;

    void writeUint32(std::ofstream &out, std::uint32_t value)
    {
        for (int i = 0; i < 4; ++i)
        {
            out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
    }

    bool readUint32(std::ifstream &in, std::uint32_t &value)
    {
        value = 0;

        for (int i = 0; i < 4; ++i)
        {
            char byte;

            if (!in.get(byte))
            {
                return false;
            }

            value |=
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(byte))
                << (i * 8);
        }

        return true;
    }

    void writeUint64(std::ofstream &out, std::uint64_t value)
    {
        for (int i = 0; i < 8; ++i)
        {
            out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
        }
    }

    bool readUint64(std::ifstream &in, std::uint64_t &value)
    {
        value = 0;

        for (int i = 0; i < 8; ++i)
        {
            char byte;

            if (!in.get(byte))
            {
                return false;
            }

            value |=
                static_cast<std::uint64_t>(
                    static_cast<unsigned char>(byte))
                << (i * 8);
        }

        return true;
    }

} // namespace

WAL::WAL(const std::string &path)
    : filePath(path)
{
}

bool WAL::appendRecord(
    std::uint8_t operation,
    SequenceNumber sequence,
    const std::string &key,
    const std::string &value)
{
    if (sequence == 0 ||
        key.size() > MAX_RECORD_SIZE ||
        value.size() > MAX_RECORD_SIZE ||
        key.size() > std::numeric_limits<std::uint32_t>::max() ||
        value.size() > std::numeric_limits<std::uint32_t>::max())
    {
        return false;
    }

    if (operation != PUT_OPERATION &&
        operation != DELETE_OPERATION)
    {
        return false;
    }

    if (operation == DELETE_OPERATION && !value.empty())
    {
        return false;
    }

    std::ofstream out(
        filePath,
        std::ios::binary | std::ios::app);

    if (!out.is_open())
    {
        return false;
    }

    out.put(static_cast<char>(operation));
    writeUint64(out, sequence);

    writeUint32(out, static_cast<std::uint32_t>(key.size()));
    writeUint32(out, static_cast<std::uint32_t>(value.size()));

    out.write(
        key.data(),
        static_cast<std::streamsize>(key.size()));

    out.write(
        value.data(),
        static_cast<std::streamsize>(value.size()));

    out.flush();

    if (!out.good())
    {
        return false;
    }

    out.close();

    return !out.fail();
}

bool WAL::appendPut(
    SequenceNumber sequence,
    const std::string &key,
    const std::string &value)
{
    return appendRecord(PUT_OPERATION, sequence, key, value);
}

bool WAL::appendDelete(
    SequenceNumber sequence,
    const std::string &key)
{
    return appendRecord(DELETE_OPERATION, sequence, key, "");
}

// Compatibility functions for existing tests.
bool WAL::appendPut(
    const std::string &key,
    const std::string &value)
{
    return appendPut(legacyNextSequence++, key, value);
}

bool WAL::appendDelete(const std::string &key)
{
    return appendDelete(legacyNextSequence++, key);
}
bool WAL::reset()
{

    std::ofstream out(
        filePath,
        std::ios::binary | std::ios::trunc);

    if (!out.is_open())
    {
        return false;
    }

    out.flush();

    if (!out.good())
    {
        return false;
    }

    out.close();

    return !out.fail();
}
bool WAL::replay(
    MemTable &table,
    SequenceNumber checkpointSequence) const
{

    std::ifstream in(filePath, std::ios::binary);

    if (!in.is_open())
    {
        return true;
    }

    std::error_code ec;

    const std::uintmax_t fileSize =
        std::filesystem::file_size(filePath, ec);

    if (ec)
    {
        return false;
    }

    std::uintmax_t offset = 0;

    auto truncateTail = [&](std::uintmax_t validSize)
    {
        in.close();

        std::error_code truncateError;

        std::filesystem::resize_file(
            filePath,
            validSize,
            truncateError);

        return !truncateError;
    };

    SequenceNumber previousSequence = 0;

    while (offset < fileSize)
    {

        const std::uintmax_t recordStart = offset;

        if (fileSize - offset < HEADER_SIZE)
        {
            return truncateTail(recordStart);
        }

        char operationByte;

        if (!in.get(operationByte))
        {
            return false;
        }

        SequenceNumber sequence;
        std::uint32_t keyLength;
        std::uint32_t valueLength;

        if (!readUint64(in, sequence) ||
            !readUint32(in, keyLength) ||
            !readUint32(in, valueLength))
        {
            return false;
        }

        offset += HEADER_SIZE;

        const auto operation =
            static_cast<std::uint8_t>(
                static_cast<unsigned char>(operationByte));

        if (operation != PUT_OPERATION &&
            operation != DELETE_OPERATION)
        {
            return false;
        }

        if (sequence == 0 ||
            sequence <= previousSequence)
        {
            return false;
        }

        if (keyLength > MAX_RECORD_SIZE ||
            valueLength > MAX_RECORD_SIZE)
        {
            return false;
        }

        if (operation == DELETE_OPERATION &&
            valueLength != 0)
        {
            return false;
        }

        const std::uint64_t payloadSize =
            static_cast<std::uint64_t>(keyLength) +
            static_cast<std::uint64_t>(valueLength);

        if (fileSize - offset < payloadSize)
        {
            return truncateTail(recordStart);
        }

        std::string key(keyLength, '\0');
        std::string value(valueLength, '\0');

        if (!in.read(
                key.data(),
                static_cast<std::streamsize>(keyLength)))
        {
            return false;
        }

        if (!in.read(
                value.data(),
                static_cast<std::streamsize>(valueLength)))
        {
            return false;
        }

        // Restore the exact sequence number.
        // Ignore records already covered by the manifest checkpoint.
        if (sequence > checkpointSequence)
        {

            if (operation == PUT_OPERATION)
            {
                if (!table.putInternal(key, value, sequence))
                {
                    return false;
                }
            }
            else
            {
                if (!table.removeInternal(key, sequence))
                {
                    return false;
                }
            }
        }

        previousSequence = sequence;
        offset += payloadSize;
    }

    return true;
}