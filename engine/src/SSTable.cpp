#include "SSTable.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace {
constexpr char SSTABLE_MAGIC[8] = {'S', 'T', 'R', 'A', 'T', 'A', 'S', '1'};
constexpr std::uint32_t MAX_ENTRY_COUNT = 10000000;
}

SSTable::SSTable(std::size_t expectedEntries)
    : bloomFilter(expectedEntries), loaded(false) {}

bool SSTable::writeUint32(std::ofstream& out, std::uint32_t value) {
    for (int i = 0; i < 4; i++) {
        out.put(static_cast<char>((value >> (i * 8)) & 0xFF));
    }

    return out.good();
}

bool SSTable::readUint32(std::ifstream& in, std::uint32_t& value) {
    value = 0;

    for (int i = 0; i < 4; i++) {
        char byte;
        if (!in.get(byte)) {
            return false;
        }

        value |= static_cast<std::uint32_t>(
                     static_cast<unsigned char>(byte))
                 << (i * 8);
    }

    return true;
}

bool SSTable::write(
    const std::string& filePath,
    const std::vector<std::pair<std::string, std::string>>& data
) {
    if (data.size() > MAX_ENTRY_COUNT ||
        data.size() > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    auto sortedData = data;
    std::sort(
        sortedData.begin(),
        sortedData.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        }
    );

    for (std::size_t i = 1; i < sortedData.size(); i++) {
        if (sortedData[i - 1].first == sortedData[i].first) {
            return false;
        }
    }

    for (const auto& entry : sortedData) {
        if (entry.first.size() > MAX_FIELD_SIZE ||
            entry.second.size() > MAX_FIELD_SIZE) {
            return false;
        }
    }

    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out.write(SSTABLE_MAGIC, sizeof(SSTABLE_MAGIC));
    if (!out.good() ||
        !writeUint32(out, static_cast<std::uint32_t>(sortedData.size()))) {
        return false;
    }

    for (const auto& entry : sortedData) {
        if (!writeUint32(out, static_cast<std::uint32_t>(entry.first.size())) ||
            !writeUint32(out, static_cast<std::uint32_t>(entry.second.size()))) {
            return false;
        }

        out.write(entry.first.data(),
                  static_cast<std::streamsize>(entry.first.size()));
        out.write(entry.second.data(),
                  static_cast<std::streamsize>(entry.second.size()));
        if (!out.good()) {
            return false;
        }
    }

    out.flush();
    return out.good();
}

bool SSTable::load(const std::string& filePath) {
    loaded = false;
    entries.clear();

    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    char magic[sizeof(SSTABLE_MAGIC)];
    if (!in.read(magic, sizeof(magic)) ||
        !std::equal(std::begin(magic), std::end(magic),
                    std::begin(SSTABLE_MAGIC))) {
        return false;
    }

    std::uint32_t entryCount;
    if (!readUint32(in, entryCount) || entryCount > MAX_ENTRY_COUNT) {
        return false;
    }

    std::vector<std::pair<std::string, std::string>> loadedEntries;
    loadedEntries.reserve(entryCount);

    for (std::uint32_t i = 0; i < entryCount; i++) {
        std::uint32_t keyLength;
        std::uint32_t valueLength;
        if (!readUint32(in, keyLength) ||
            !readUint32(in, valueLength) ||
            keyLength > MAX_FIELD_SIZE ||
            valueLength > MAX_FIELD_SIZE) {
            return false;
        }

        std::string key(keyLength, '\0');
        std::string value(valueLength, '\0');
        if (!in.read(key.data(), static_cast<std::streamsize>(keyLength)) ||
            !in.read(value.data(), static_cast<std::streamsize>(valueLength))) {
            return false;
        }

        if (!loadedEntries.empty() && loadedEntries.back().first >= key) {
            return false;
        }

        loadedEntries.emplace_back(std::move(key), std::move(value));
    }

    if (in.peek() != std::char_traits<char>::eof()) {
        return false;
    }

    BloomFilter newFilter(loadedEntries.empty() ? 1 : loadedEntries.size());
    for (const auto& entry : loadedEntries) {
        newFilter.add(entry.first);
    }

    entries = std::move(loadedEntries);
    bloomFilter = std::move(newFilter);
    loaded = true;
    return true;
}

bool SSTable::get(const std::string& key, std::string& value) const {
    if (!loaded || !bloomFilter.mightContain(key)) {
        return false;
    }

    const auto it = std::lower_bound(
        entries.begin(),
        entries.end(),
        key,
        [](const auto& entry, const std::string& target) {
            return entry.first < target;
        }
    );

    if (it == entries.end() || it->first != key) {
        return false;
    }

    value = it->second;
    return true;
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
