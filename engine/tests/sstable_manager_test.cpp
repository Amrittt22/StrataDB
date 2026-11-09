#include <gtest/gtest.h>

#include "SSTableManager.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class SSTableManagerTest : public ::testing::Test {
protected:
    const fs::path testDirectory =
        fs::temp_directory_path() / "stratadb_manager_tests";

    void SetUp() override {
        fs::remove_all(testDirectory);
        fs::create_directories(testDirectory);
    }

    void TearDown() override {
        fs::remove_all(testDirectory);
    }

    std::vector<SSTableEntry> makeEntries(
        const std::string& key,
        const std::string& value
    ) {
        return {{key, value, false}};
    }
};

TEST_F(SSTableManagerTest, StartsWithNoTables) {
    SSTableManager manager(testDirectory.string());

    EXPECT_TRUE(manager.loadExisting());
    EXPECT_TRUE(manager.isLoaded());
    EXPECT_EQ(manager.count(), 0);
}

TEST_F(SSTableManagerTest, CannotAddBeforeLoading) {
    SSTableManager manager(testDirectory.string());

    EXPECT_FALSE(
        manager.addSSTable(makeEntries("key", "value"))
    );
}

TEST_F(SSTableManagerTest, AddsNewSSTable) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    std::uint64_t id = 0;

    EXPECT_TRUE(
        manager.addSSTable(makeEntries("name", "StrataDB"), &id)
    );

    EXPECT_EQ(id, 1);
    EXPECT_EQ(manager.count(), 1);
    EXPECT_TRUE(fs::exists(
        testDirectory / "sstable_1.sst"
    ));
}

TEST_F(SSTableManagerTest, AssignsIncreasingIds) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    std::uint64_t firstId = 0;
    std::uint64_t secondId = 0;

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("a", "first"),
        &firstId
    ));

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("b", "second"),
        &secondId
    ));

    EXPECT_EQ(firstId, 1);
    EXPECT_EQ(secondId, 2);

    const auto ids = manager.getSSTableIds();

    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 2);
    EXPECT_EQ(ids[1], 1);
}

TEST_F(SSTableManagerTest, FindsValueInSSTable) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("language", "C++")
    ));

    std::string value;

    EXPECT_EQ(
        manager.lookup("language", value),
        LookupResult::VALUE_FOUND
    );

    EXPECT_EQ(value, "C++");
}

TEST_F(SSTableManagerTest, SearchesOlderSSTableIfNewerDoesNotContainKey) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("older-key", "older-value")
    ));

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("newer-key", "newer-value")
    ));

    std::string value;

    EXPECT_EQ(
        manager.lookup("older-key", value),
        LookupResult::VALUE_FOUND
    );

    EXPECT_EQ(value, "older-value");
}

TEST_F(SSTableManagerTest, NewerValueOverridesOlderValue) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("key", "old-value")
    ));

    ASSERT_TRUE(manager.addSSTable(
        makeEntries("key", "new-value")
    ));

    std::string value;

    EXPECT_EQ(
        manager.lookup("key", value),
        LookupResult::VALUE_FOUND
    );

    EXPECT_EQ(value, "new-value");
}

TEST_F(SSTableManagerTest, NewerTombstoneHidesOlderValue) {
    SSTableManager manager(testDirectory.string());

    ASSERT_TRUE(manager.loadExisting());

    ASSERT_TRUE(manager.addSSTable({
        {"key", "old-value", false}
    }));

    ASSERT_TRUE(manager.addSSTable({
        {"key", "", true}
    }));

    std::string value;

    EXPECT_EQ(
        manager.lookup("key", value),
        LookupResult::TOMBSTONE_FOUND
    );
}

TEST_F(SSTableManagerTest, LoadsExistingTablesAfterRestart) {
    {
        SSTableManager manager(testDirectory.string());

        ASSERT_TRUE(manager.loadExisting());

        ASSERT_TRUE(manager.addSSTable(
            makeEntries("key", "persisted-value")
        ));

        ASSERT_TRUE(manager.addSSTable(
            makeEntries("another-key", "another-value")
        ));
    }

    // Simulate a new StrataDB process.
    SSTableManager recoveredManager(testDirectory.string());

    ASSERT_TRUE(recoveredManager.loadExisting());

    EXPECT_EQ(recoveredManager.count(), 2);

    std::string value;

    EXPECT_EQ(
        recoveredManager.lookup("key", value),
        LookupResult::VALUE_FOUND
    );

    EXPECT_EQ(value, "persisted-value");

    std::uint64_t newId = 0;

    ASSERT_TRUE(recoveredManager.addSSTable(
        makeEntries("third-key", "third-value"),
        &newId
    ));

    EXPECT_EQ(newId, 3);
}

TEST_F(SSTableManagerTest, IgnoresUnrelatedFiles) {
    {
        std::ofstream file(testDirectory / "notes.txt");
        file << "not an SSTable";
    }

    SSTableManager manager(testDirectory.string());

    EXPECT_TRUE(manager.loadExisting());
    EXPECT_EQ(manager.count(), 0);
}