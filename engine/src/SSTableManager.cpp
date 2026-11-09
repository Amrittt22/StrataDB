#include "SSTableManager.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <limits>
#include <map>
#include <system_error>

namespace fs = std::filesystem;

SSTableManager::SSTableManager(const std::string& directory)
    : directory(directory),
      nextId(1),
      loaded(false) {
}

bool SSTableManager::parseTableId(
    const std::string& filename,
    std::uint64_t& id
) {
    const std::string prefix = "sstable_";
    const std::string suffix = ".sst";

    if (filename.size() <= prefix.size() + suffix.size()) {
        return false;
    }

    if (filename.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }

    if (filename.compare(
            filename.size() - suffix.size(),
            suffix.size(),
            suffix
        ) != 0) {
        return false;
    }

    const std::string idText = filename.substr(
        prefix.size(),
        filename.size() - prefix.size() - suffix.size()
    );

    std::uint64_t parsedId = 0;

    const char* begin = idText.data();
    const char* end = begin + idText.size();

    auto result = std::from_chars(begin, end, parsedId);

    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    id = parsedId;
    return true;
}

std::string SSTableManager::makeFilePath(
    std::uint64_t id
) const {
    return (
        fs::path(directory) /
        ("sstable_" + std::to_string(id) + ".sst")
    ).string();
}

bool SSTableManager::loadExisting() {
    try {
        fs::create_directories(directory);

        std::vector<ManagedTable> loadedTables;
        std::uint64_t maximumId = 0;

        for (const auto& entry : fs::directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::string filename =
                entry.path().filename().string();

            std::uint64_t id = 0;

            if (!parseTableId(filename, id)) {
                continue;
            }

            auto table = std::make_unique<SSTable>();

            if (!table->load(entry.path().string())) {
                return false;
            }

            loadedTables.push_back({
                id,
                entry.path().string(),
                std::move(table)
            });

            maximumId = std::max(maximumId, id);
        }

        // Explicitly sort by sequence ID, not directory order.
        std::sort(
            loadedTables.begin(),
            loadedTables.end(),
            [](const ManagedTable& a, const ManagedTable& b) {
                return a.id > b.id;
            }
        );

        if (maximumId == std::numeric_limits<std::uint64_t>::max()) {
            return false;
        }

        tables = std::move(loadedTables);
        nextId = maximumId + 1;
        loaded = true;

        return true;
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

bool SSTableManager::addSSTable(
    const std::vector<SSTableEntry>& entries,
    std::uint64_t* createdId
) {
    if (!loaded) {
        return false;
    }

    if (nextId == std::numeric_limits<std::uint64_t>::max()) {
        return false;
    }

    const std::uint64_t id = nextId;

    const std::string finalPath = makeFilePath(id);
    const std::string temporaryPath = finalPath + ".tmp";

    try {
        // Do not overwrite an existing table.
        if (fs::exists(finalPath) || fs::exists(temporaryPath)) {
            return false;
        }

        if (!SSTable::writeEntries(temporaryPath, entries)) {
            return false;
        }

        auto table = std::make_unique<SSTable>();

        if (!table->load(temporaryPath)) {
            fs::remove(temporaryPath);
            return false;
        }

        // Rename the completed temporary file into its final name.
        std::error_code error;
        fs::rename(temporaryPath, finalPath, error);

        if (error) {
            fs::remove(temporaryPath);
            return false;
        }

        tables.insert(
            tables.begin(),
            ManagedTable{
                id,
                finalPath,
                std::move(table)
            }
        );

        ++nextId;

        if (createdId != nullptr) {
            *createdId = id;
        }

        return true;
    }
    catch (const fs::filesystem_error&) {
        return false;
    }
}

LookupResult SSTableManager::lookup(
    const std::string& key,
    std::string& value
) const {
    if (!loaded) {
        return LookupResult::NOT_FOUND;
    }

    const SSTableEntry* newestEntry = nullptr;

    for (const auto& managedTable : tables) {
        if (!managedTable.table->mightContain(key)) {
            continue;
        }

        const auto& entries = managedTable.table->getEntries();
        auto it = std::lower_bound(
            entries.begin(),
            entries.end(),
            key,
            [](const SSTableEntry& entry, const std::string& target) {
                return entry.key < target;
            }
        );

        if (it == entries.end() || it->key != key) {
            continue;
        }

        if (newestEntry == nullptr ||
            it->sequence > newestEntry->sequence) {
            newestEntry = &(*it);
        }
    }

    if (newestEntry == nullptr) {
        return LookupResult::NOT_FOUND;
    }

    if (newestEntry->isTombstone) {
        return LookupResult::TOMBSTONE_FOUND;
    }

    value = newestEntry->value;
    return LookupResult::VALUE_FOUND;
}

std::size_t SSTableManager::count() const {
    return tables.size();
}

std::vector<SSTableEntry>
SSTableManager::getEntriesNewestFirst() const {
    std::vector<SSTableEntry> result;

    if (!loaded) {
        return result;
    }

    for (const auto& managedTable : tables) {
        const auto& entries = managedTable.table->getEntries();

        result.insert(
            result.end(),
            entries.begin(),
            entries.end()
        );
    }

    return result;
}

bool SSTableManager::compactAll() {
    if (!loaded) {
        return false;
    }

    if (tables.size() <= 1) {
        return true;
    }

    std::map<std::string, SSTableEntry> latest;

for (const auto& managed : tables) {

    for (const auto& entry : managed.table->getEntries()) {

        auto it = latest.find(entry.key);

        if (it == latest.end() ||
            entry.sequence > it->second.sequence) {

            latest[entry.key] = entry;
        }
    }
}

    std::vector<SSTableEntry> merged;
    merged.reserve(latest.size());

    for (auto& item : latest) {
        merged.push_back(std::move(item.second));
    }

    const std::size_t oldCount = tables.size();

    if (!addSSTable(merged)) {
        return false;
    }

    for (std::size_t i = 1; i <= oldCount; ++i) {
        std::error_code error;
        fs::remove(tables[i].filePath, error);

        if (error) {
            return false;
        }
    }

    std::vector<ManagedTable> retained;
    retained.push_back(std::move(tables.front()));
    tables = std::move(retained);

    return true;
}

std::vector<std::uint64_t> SSTableManager::getSSTableIds() const {
    std::vector<std::uint64_t> ids;

    for (const auto& table : tables) {
        ids.push_back(table.id);
    }

    return ids;
}

bool SSTableManager::isLoaded() const {
    return loaded;
}