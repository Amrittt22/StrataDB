#include <gtest/gtest.h>

#include "../include/Database.h"
#include "../include/SSTable.h"
#include "../include/MemTable.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class DatabaseTest : public ::testing::Test {
protected:
    fs::path testDirectory =
        fs::temp_directory_path() / "stratadb_database_tests";

    void SetUp() override {
        fs::remove_all(testDirectory);
        fs::create_directories(testDirectory);
    }

    void TearDown() override {
        fs::remove_all(testDirectory);
    }

    fs::path sstableDirectory() {
        return testDirectory / "sstables";
    }

    bool createOlderSSTable(
        const std::string& key,
        const std::string& value
    ) {
        fs::create_directories(sstableDirectory());

        return SSTable::writeEntries(
            (sstableDirectory() / "sstable_1.sst").string(),
            {{key, value, false}}
        );
    }
};

TEST_F(DatabaseTest, OpensSuccessfully) {
    Database db(testDirectory.string());

    EXPECT_TRUE(db.open());
    EXPECT_TRUE(db.isOpen());
}

TEST_F(DatabaseTest, PutAndGet) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    EXPECT_TRUE(db.put("name", "StrataDB"));

    std::string value;

    EXPECT_TRUE(db.get("name", value));
    EXPECT_EQ(value, "StrataDB");
}

TEST_F(DatabaseTest, ReturnsFalseForMissingKey) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    std::string value;

    EXPECT_FALSE(db.get("missing", value));
}

TEST_F(DatabaseTest, UpdatesExistingValue) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("language", "Java"));
    ASSERT_TRUE(db.put("language", "C++"));

    std::string value;

    ASSERT_TRUE(db.get("language", value));
    EXPECT_EQ(value, "C++");
}

TEST_F(DatabaseTest, RemovesExistingKey) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("name", "StrataDB"));

    EXPECT_TRUE(db.remove("name"));

    std::string value;

    EXPECT_FALSE(db.get("name", value));
}

TEST_F(DatabaseTest, RemoveMissingKeyReturnsFalse) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    EXPECT_FALSE(db.remove("missing"));
}

TEST_F(DatabaseTest, RecoversDataAfterRestart) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("name", "StrataDB"));
    }

    Database recoveredDb(testDirectory.string());

    ASSERT_TRUE(recoveredDb.open());

    std::string value;

    EXPECT_TRUE(recoveredDb.get("name", value));
    EXPECT_EQ(value, "StrataDB");
}

TEST_F(DatabaseTest, ReadsValueFromSSTable) {
    ASSERT_TRUE(createOlderSSTable("name", "StrataDB"));

    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    std::string value;

    EXPECT_TRUE(db.get("name", value));
    EXPECT_EQ(value, "StrataDB");
}

TEST_F(DatabaseTest, MemTableValueOverridesSSTableValue) {
    ASSERT_TRUE(createOlderSSTable("name", "Old"));

    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("name", "New"));

    std::string value;

    EXPECT_TRUE(db.get("name", value));
    EXPECT_EQ(value, "New");
}

TEST_F(DatabaseTest, MemTableTombstoneHidesSSTableValue) {
    ASSERT_TRUE(createOlderSSTable("name", "Old"));

    // Write a deletion to the WAL.
    {
        MemTable temporaryMemTable(
            (testDirectory / "wal.log").string()
        );

        EXPECT_FALSE(temporaryMemTable.remove("name"));
    }

    // A new database instance recovers the tombstone from the WAL.
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    std::string value;

    EXPECT_FALSE(db.get("name", value));
}

TEST_F(DatabaseTest, PutAfterDeleteMakesKeyVisibleAgain) {
    ASSERT_TRUE(createOlderSSTable("name", "Old"));

    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.remove("name"));
    ASSERT_TRUE(db.put("name", "New"));

    std::string value;

    EXPECT_TRUE(db.get("name", value));
    EXPECT_EQ(value, "New");
}

TEST_F(DatabaseTest, FlushWritesMemTableToSSTable) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("apple", "red"));
    ASSERT_TRUE(db.put("banana", "yellow"));

    EXPECT_EQ(db.sstableCount(), 0);

    ASSERT_TRUE(db.flushMemTable());

    EXPECT_EQ(db.sstableCount(), 1);

    std::string value;

    EXPECT_TRUE(db.get("apple", value));
    EXPECT_EQ(value, "red");

    EXPECT_TRUE(db.get("banana", value));
    EXPECT_EQ(value, "yellow");
}

TEST_F(DatabaseTest, FlushPreservesTombstones) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("apple", "red"));
    ASSERT_TRUE(db.flushMemTable());

    ASSERT_TRUE(db.remove("apple"));
    ASSERT_TRUE(db.flushMemTable());

    std::string value;

    EXPECT_FALSE(db.get("apple", value));
}

TEST_F(DatabaseTest, FlushClearsSingleEntryAfterCheckpoint) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("apple", "red"));

    ASSERT_TRUE(db.flushMemTable());

    EXPECT_EQ(db.memTableSize(), 0);

    std::string value;

    EXPECT_TRUE(db.get("apple", value));
    EXPECT_EQ(value, "red");
}

TEST_F(DatabaseTest, EmptyFlushSucceeds) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    EXPECT_TRUE(db.flushMemTable());

    EXPECT_EQ(db.sstableCount(), 0);
}

TEST_F(DatabaseTest, FlushPersistsDataAcrossRestart) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());

        ASSERT_TRUE(db.put("apple", "red"));
        ASSERT_TRUE(db.flushMemTable());
    }

    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());

        std::string value;

        EXPECT_TRUE(db.get("apple", value));
        EXPECT_EQ(value, "red");
    }
}

TEST_F(DatabaseTest, RangeScanReturnsSortedVisibleValues) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("b", "two"));
    ASSERT_TRUE(db.put("a", "one"));
    ASSERT_TRUE(db.put("c", "three"));
    ASSERT_TRUE(db.put("d", "four"));

    const auto rows = db.scan("b", "c");

    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0], std::make_pair(
        std::string("b"),
        std::string("two")
    ));
    EXPECT_EQ(rows[1], std::make_pair(
        std::string("c"),
        std::string("three")
    ));
}

TEST_F(DatabaseTest, RangeScanHonorsMemTableTombstones) {
    ASSERT_TRUE(createOlderSSTable("b", "old"));

    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.remove("b"));
    ASSERT_TRUE(db.put("c", "visible"));

    const auto rows = db.scan("a", "z");

    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].first, "c");
    EXPECT_EQ(rows[0].second, "visible");
}

TEST_F(DatabaseTest, CompactionKeepsNewestValue) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("key", "old"));
    ASSERT_TRUE(db.flushMemTable());
    ASSERT_TRUE(db.put("key", "new"));
    ASSERT_TRUE(db.flushMemTable());

    ASSERT_TRUE(db.compact());

    std::string value;
    ASSERT_TRUE(db.get("key", value));
    EXPECT_EQ(value, "new");
    EXPECT_EQ(db.sstableCount(), 1u);
}

TEST_F(DatabaseTest, FlushClearsMemTableAfterCheckpoint) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("name", "Amrit"));
    ASSERT_TRUE(db.put("city", "Bangalore"));
    ASSERT_EQ(db.memTableSize(), 2u);

    ASSERT_TRUE(db.flushMemTable());

    EXPECT_EQ(db.memTableSize(), 0u);
    EXPECT_EQ(db.sstableCount(), 1u);
}

TEST_F(DatabaseTest, CheckpointRecoveryDoesNotLoseFlushedData) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("a", "one"));
        ASSERT_TRUE(db.put("b", "two"));
        ASSERT_TRUE(db.flushMemTable());
        EXPECT_EQ(db.memTableSize(), 0u);
    }

    {
        Database recovered(testDirectory.string());

        ASSERT_TRUE(recovered.open());

        std::string value;
        EXPECT_TRUE(recovered.get("a", value));
        EXPECT_EQ(value, "one");
        EXPECT_TRUE(recovered.get("b", value));
        EXPECT_EQ(value, "two");
        EXPECT_EQ(recovered.memTableSize(), 0u);
    }
}

TEST_F(DatabaseTest, WritesAfterCheckpointSurviveRestart) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("a", "one"));
        ASSERT_TRUE(db.flushMemTable());
        ASSERT_TRUE(db.put("b", "two"));
    }

    {
        Database recovered(testDirectory.string());

        ASSERT_TRUE(recovered.open());

        std::string value;
        EXPECT_TRUE(recovered.get("a", value));
        EXPECT_EQ(value, "one");
        EXPECT_TRUE(recovered.get("b", value));
        EXPECT_EQ(value, "two");
    }
}

TEST_F(DatabaseTest, DeleteAfterCheckpointSurvivesRestart) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("a", "one"));
        ASSERT_TRUE(db.flushMemTable());
        ASSERT_TRUE(db.remove("a"));
    }

    {
        Database recovered(testDirectory.string());

        ASSERT_TRUE(recovered.open());

        std::string value;
        EXPECT_FALSE(recovered.get("a", value));
    }
}

TEST_F(DatabaseTest, AutomaticallyFlushesAtThreshold) {
    Database db(testDirectory.string(), 2);

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("a", "one"));

    EXPECT_EQ(db.memTableSize(), 1u);
    EXPECT_EQ(db.sstableCount(), 0u);

    ASSERT_TRUE(db.put("b", "two"));

    EXPECT_EQ(db.memTableSize(), 0u);
    EXPECT_EQ(db.sstableCount(), 1u);
    EXPECT_TRUE(db.wasLastAutoFlushSuccessful());

    std::string value;
    EXPECT_TRUE(db.get("a", value));
    EXPECT_EQ(value, "one");
    EXPECT_TRUE(db.get("b", value));
    EXPECT_EQ(value, "two");
}

TEST_F(DatabaseTest, UpdatingExistingKeyDoesNotTriggerEarlyFlush) {
    Database db(testDirectory.string(), 2);

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("a", "one"));
    ASSERT_TRUE(db.put("a", "updated"));

    EXPECT_EQ(db.memTableSize(), 1u);
    EXPECT_EQ(db.sstableCount(), 0u);

    ASSERT_TRUE(db.put("b", "two"));

    EXPECT_EQ(db.memTableSize(), 0u);
    EXPECT_EQ(db.sstableCount(), 1u);

    std::string value;
    EXPECT_TRUE(db.get("a", value));
    EXPECT_EQ(value, "updated");
}

TEST_F(DatabaseTest, TombstonesCountTowardFlushThreshold) {
    Database db(testDirectory.string(), 2);

    ASSERT_TRUE(db.open());
    ASSERT_TRUE(db.put("a", "one"));

    EXPECT_FALSE(db.remove("missing"));
    EXPECT_EQ(db.memTableSize(), 0u);
    EXPECT_EQ(db.sstableCount(), 1u);
    EXPECT_TRUE(db.wasLastAutoFlushSuccessful());

    std::string value;
    EXPECT_FALSE(db.get("missing", value));

    ASSERT_TRUE(db.put("b", "two"));

    EXPECT_EQ(db.memTableSize(), 1u);
    EXPECT_EQ(db.sstableCount(), 1u);
}

TEST_F(DatabaseTest, AutomaticFlushSurvivesRestart) {
    {
        Database db(testDirectory.string(), 2);

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("a", "one"));
        ASSERT_TRUE(db.put("b", "two"));
        EXPECT_EQ(db.memTableSize(), 0u);
    }

    {
        Database recovered(testDirectory.string(), 2);

        ASSERT_TRUE(recovered.open());

        std::string value;
        EXPECT_TRUE(recovered.get("a", value));
        EXPECT_EQ(value, "one");
        EXPECT_TRUE(recovered.get("b", value));
        EXPECT_EQ(value, "two");
    }
}

TEST_F(DatabaseTest, CompactionPreservesTombstoneAfterRestart) {
    {
        Database db(testDirectory.string());

        ASSERT_TRUE(db.open());
        ASSERT_TRUE(db.put("key", "old"));
        ASSERT_TRUE(db.flushMemTable());
        ASSERT_TRUE(db.remove("key"));
        ASSERT_TRUE(db.flushMemTable());
        ASSERT_TRUE(db.compact());
        EXPECT_EQ(db.sstableCount(), 1u);
    }

    Database recovered(testDirectory.string());

    ASSERT_TRUE(recovered.open());

    std::string value;
    EXPECT_FALSE(recovered.get("key", value));
}

TEST_F(DatabaseTest, CompactionRetainsLatestValuesAcrossMultipleTables) {
    Database db(testDirectory.string());

    ASSERT_TRUE(db.open());

    ASSERT_TRUE(db.put("a", "a1"));
    ASSERT_TRUE(db.put("b", "b1"));
    ASSERT_TRUE(db.flushMemTable());

    ASSERT_TRUE(db.put("a", "a2"));
    ASSERT_TRUE(db.put("c", "c1"));
    ASSERT_TRUE(db.flushMemTable());

    ASSERT_TRUE(db.put("a", "a3"));
    ASSERT_TRUE(db.put("b", "b2"));
    ASSERT_TRUE(db.flushMemTable());
    ASSERT_TRUE(db.compact());

    std::string value;
    ASSERT_TRUE(db.get("a", value));
    EXPECT_EQ(value, "a3");
    ASSERT_TRUE(db.get("b", value));
    EXPECT_EQ(value, "b2");
    ASSERT_TRUE(db.get("c", value));
    EXPECT_EQ(value, "c1");
    EXPECT_EQ(db.sstableCount(), 1u);
}