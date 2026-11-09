#include "SSTable.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <utility>

namespace {

constexpr char SSTABLE_MAGIC[8] = {
    'S', 'T', 'R', 'A', 'T', 'A', 'S', '3'
};

constexpr std::uint8_t VALUE_ENTRY = 0;
constexpr std::uint8_t TOMBSTONE_ENTRY = 1;

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

}

SSTable::SSTable(std::size_t expectedEntries)
    : bloomFilter(expectedEntries),
      loaded(false) {
}

bool SSTable::writeUint32(
    std::ofstream& out,
    std::uint32_t value
) {
    for (int i = 0; i < 4; ++i) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
    }

    return out.good();
}

bool SSTable::readUint32(
    std::ifstream& in,
    std::uint32_t& value
) {
    value = 0;

    for (int i = 0; i < 4; ++i) {
        char byte;

        if (!in.get(byte)) {
            return false;
        }

        value |=
            static_cast<std::uint32_t>(
                static_cast<unsigned char>(byte)
            ) << (i * 8);
    }

    return true;
}

bool SSTable::writeEntries(
    const std::string& filePath,
    const std::vector<SSTableEntry>& data
) {
    if (data.size() > MAX_ENTRY_COUNT ||
        data.size() > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

   auto sortedData = data;

std::sort(
    sortedData.begin(),
    sortedData.end(),
    [](const SSTableEntry& a, const SSTableEntry& b) {
        return a.key < b.key;
    }
);

// Backward compatibility:
// Older code and tests create SSTableEntry with three fields,
// leaving sequence = 0. Assign valid sequences to those entries.
SequenceNumber maxSequence = 0;

for (const auto& entry : sortedData) {
    maxSequence = std::max(maxSequence, entry.sequence);
}

bool hasMissingSequence = false;

for (const auto& entry : sortedData) {
    if (entry.sequence == 0) {
        hasMissingSequence = true;
        break;
    }
}

if (hasMissingSequence &&
    maxSequence == std::numeric_limits<SequenceNumber>::max()) {
    return false;
}

SequenceNumber nextFallbackSequence = maxSequence + 1;

for (auto& entry : sortedData) {
    if (entry.sequence == 0) {
        if (nextFallbackSequence == 0) {
            return false;
        }

        entry.sequence = nextFallbackSequence++;

        if (nextFallbackSequence == 0 &&
            &entry != &sortedData.back()) {
            return false;
        }
    }
}

    for (std::size_t i = 0; i < sortedData.size(); ++i) {

        const auto& entry = sortedData[i];

        if (entry.key.size() > MAX_FIELD_SIZE ||
            entry.value.size() > MAX_FIELD_SIZE ||
            entry.sequence == 0) {
            return false;
        }

        if (entry.isTombstone && !entry.value.empty()) {
            return false;
        }

        if (i > 0 &&
            sortedData[i - 1].key == entry.key) {
            return false;
        }
    }

    std::ofstream out(
        filePath,
        std::ios::binary | std::ios::trunc
    );

    if (!out.is_open()) {
        return false;
    }

    out.write(SSTABLE_MAGIC, sizeof(SSTABLE_MAGIC));

    if (!out.good()) {
        return false;
    }

    if (!writeUint32(
            out,
            static_cast<std::uint32_t>(sortedData.size())
        )) {
        return false;
    }

    for (const auto& entry : sortedData) {

        if (!writeUint32(
                out,
                static_cast<std::uint32_t>(entry.key.size())
            ) ||
            !writeUint32(
                out,
                static_cast<std::uint32_t>(entry.value.size())
            )) {
            return false;
        }

        out.put(
            static_cast<char>(
                entry.isTombstone
                    ? TOMBSTONE_ENTRY
                    : VALUE_ENTRY
            )
        );

        writeUint64(out, entry.sequence);

        out.write(
            entry.key.data(),
            static_cast<std::streamsize>(entry.key.size())
        );

        out.write(
            entry.value.data(),
            static_cast<std::streamsize>(entry.value.size())
        );

        if (!out.good()) {
            return false;
        }
    }

    out.flush();

    if (!out.good()) {
        return false;
    }

    out.close();

    return !out.fail();
}

bool SSTable::write(
    const std::string& filePath,
    const std::vector<std::pair<std::string, std::string>>& data
) {
    std::vector<SSTableEntry> converted;

    converted.reserve(data.size());

    SequenceNumber sequence = 1;

    for (const auto& entry : data) {
        converted.push_back({
            entry.first,
            entry.second,
            false,
            sequence++
        });
    }

    return writeEntries(filePath, converted);
}

bool SSTable::load(const std::string& filePath) {

    loaded = false;
    entries.clear();

    std::ifstream in(filePath, std::ios::binary);

    if (!in.is_open()) {
        return false;
    }

    char magic[8];

    if (!in.read(magic, sizeof(magic))) {
        return false;
    }

    if (!std::equal(
            std::begin(magic),
            std::end(magic),
            std::begin(SSTABLE_MAGIC)
        )) {
        return false;
    }

    std::uint32_t entryCount;

    if (!readUint32(in, entryCount) ||
        entryCount > MAX_ENTRY_COUNT) {
        return false;
    }

    std::vector<SSTableEntry> loadedEntries;
    loadedEntries.reserve(entryCount);

    for (std::uint32_t i = 0; i < entryCount; ++i) {

        std::uint32_t keyLength;
        std::uint32_t valueLength;

        if (!readUint32(in, keyLength) ||
            !readUint32(in, valueLength)) {
            return false;
        }

        if (keyLength > MAX_FIELD_SIZE ||
            valueLength > MAX_FIELD_SIZE) {
            return false;
        }

        char typeByte;

        if (!in.get(typeByte)) {
            return false;
        }

        const auto type =
            static_cast<std::uint8_t>(
                static_cast<unsigned char>(typeByte)
            );

        if (type != VALUE_ENTRY &&
            type != TOMBSTONE_ENTRY) {
            return false;
        }

        if (type == TOMBSTONE_ENTRY && valueLength != 0) {
            return false;
        }

        SequenceNumber sequence;

        if (!readUint64(in, sequence) || sequence == 0) {
            return false;
        }

        std::string key(keyLength, '\0');
        std::string value(valueLength, '\0');

        if (!in.read(
                key.data(),
                static_cast<std::streamsize>(keyLength)
            ) ||
            !in.read(
                value.data(),
                static_cast<std::streamsize>(valueLength)
            )) {
            return false;
        }

        if (!loadedEntries.empty() &&
            loadedEntries.back().key >= key) {
            return false;
        }

        loadedEntries.push_back({
            std::move(key),
            std::move(value),
            type == TOMBSTONE_ENTRY,
            sequence
        });
    }

    if (in.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    BloomFilter newFilter(
        loadedEntries.empty() ? 1 : loadedEntries.size()
    );

    for (const auto& entry : loadedEntries) {
        newFilter.add(entry.key);
    }

    entries = std::move(loadedEntries);
    bloomFilter = std::move(newFilter);
    loaded = true;

    return true;
}

LookupResult SSTable::lookup(
    const std::string& key,
    std::string& value
) const {

    if (!loaded || !bloomFilter.mightContain(key)) {
        return LookupResult::NOT_FOUND;
    }

    auto it = std::lower_bound(
        entries.begin(),
        entries.end(),
        key,
        [](const SSTableEntry& entry, const std::string& target) {
            return entry.key < target;
        }
    );

    if (it == entries.end() || it->key != key) {
        return LookupResult::NOT_FOUND;
    }

    if (it->isTombstone) {
        return LookupResult::TOMBSTONE_FOUND;
    }

    value = it->value;

    return LookupResult::VALUE_FOUND;
}

bool SSTable::get(
    const std::string& key,
    std::string& value
) const {
    return lookup(key, value) == LookupResult::VALUE_FOUND;
}

bool SSTable::mightContain(const std::string& key) const {
    return loaded && bloomFilter.mightContain(key);
}

std::size_t SSTable::size() const {
    return entries.size();
}

bool SSTable::isLoaded() const {
    return loaded;
}

const std::vector<SSTableEntry>& SSTable::getEntries() const {
    return entries;
}