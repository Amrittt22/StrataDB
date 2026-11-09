
#include <gtest/gtest.h>
#include <filesystem>
#include <string>

#include "MemTable.h"

namespace fs = std::filesystem;

namespace {
    const std::string TEST_WAL = "memtable_test.wal";

    void cleanWal() {
        fs::remove(TEST_WAL);
    }
}

// Original MemTable test fixture.
class MemTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        cleanWal();
    }

    void TearDown() override {
        cleanWal();
    }
};

// Separate fixture for sequence-number tests.
class MemTableSequenceTest : public ::testing::Test {
protected:
    fs::path testDirectory =
        fs::temp_directory_path() / "stratadb_sequence_tests";

    void SetUp() override {
        fs::remove_all(testDirectory);
        fs::create_directories(testDirectory);
    }

    void TearDown() override {
        fs::remove_all(testDirectory);
    }

    std::string walPath(const std::string& filename) {
        return (testDirectory / filename).string();
    }
};

// 1. Insert and retrieve.
TEST_F(MemTableTest, InsertAndRetrieve) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "Amritanshu"));

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "Amritanshu");
}

// 2. Update existing key.
TEST_F(MemTableTest, UpdateExistingKey) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "OldValue"));
    EXPECT_TRUE(table.put("name", "NewValue"));

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "NewValue");

    EXPECT_EQ(table.size(), 1);
}

// 3. Retrieve missing key.
TEST_F(MemTableTest, RetrieveMissingKey) {
    MemTable table(TEST_WAL);

    std::string value;

    EXPECT_FALSE(table.get("missing", value));
}

// 4. Delete existing key.
TEST_F(MemTableTest, DeleteExistingKey) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "StrataDB"));
    EXPECT_TRUE(table.remove("name"));

    std::string value;

    EXPECT_FALSE(table.get("name", value));
    EXPECT_EQ(table.size(), 0);
}

// 5. Delete missing key.
TEST_F(MemTableTest, DeleteMissingKey) {
    MemTable table(TEST_WAL);

    EXPECT_FALSE(table.remove("missing"));
    EXPECT_EQ(table.size(), 0);
}

// 6. Multiple entries.
TEST_F(MemTableTest, MultipleEntries) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "StrataDB"));
    EXPECT_TRUE(table.put("language", "C++"));
    EXPECT_TRUE(table.put("database", "LSM Tree"));

    EXPECT_EQ(table.size(), 3);

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "StrataDB");

    EXPECT_TRUE(table.get("language", value));
    EXPECT_EQ(value, "C++");

    EXPECT_TRUE(table.get("database", value));
    EXPECT_EQ(value, "LSM Tree");
}

// 7. Initially empty.
TEST_F(MemTableTest, EmptyInitially) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);
}

// 8. WAL persistence after reopening MemTable.
TEST_F(MemTableTest, RecoversDataAfterRestart) {
    {
        MemTable table(TEST_WAL);

        EXPECT_TRUE(table.put("name", "StrataDB"));
        EXPECT_TRUE(table.put("language", "C++"));

        EXPECT_EQ(table.size(), 2);
    }

    {
        MemTable recoveredTable(TEST_WAL);

        std::string value;

        EXPECT_TRUE(recoveredTable.get("name", value));
        EXPECT_EQ(value, "StrataDB");

        EXPECT_TRUE(recoveredTable.get("language", value));
        EXPECT_EQ(value, "C++");

        EXPECT_EQ(recoveredTable.size(), 2);
    }
}

// 9. WAL recovery preserves updates.
TEST_F(MemTableTest, RecoversUpdatedValue) {
    {
        MemTable table(TEST_WAL);

        EXPECT_TRUE(table.put("name", "OldValue"));
        EXPECT_TRUE(table.put("name", "NewValue"));
    }

    {
        MemTable recoveredTable(TEST_WAL);

        std::string value;

        EXPECT_TRUE(recoveredTable.get("name", value));
        EXPECT_EQ(value, "NewValue");

        EXPECT_EQ(recoveredTable.size(), 1);
    }
}

// 10. WAL recovery preserves deletions.
TEST_F(MemTableTest, RecoversDeletedKey) {
    {
        MemTable table(TEST_WAL);

        EXPECT_TRUE(table.put("name", "StrataDB"));
        EXPECT_TRUE(table.remove("name"));

        EXPECT_TRUE(table.empty());
    }

    {
        MemTable recoveredTable(TEST_WAL);

        std::string value;

        EXPECT_FALSE(recoveredTable.get("name", value));
        EXPECT_TRUE(recoveredTable.empty());
    }
}

// 11. Delete creates tombstone.
TEST_F(MemTableTest, DeleteCreatesTombstone) {
    MemTable table(TEST_WAL);

    ASSERT_TRUE(table.put("apple", "red"));
    ASSERT_TRUE(table.remove("apple"));

    EXPECT_FALSE(table.getAllTombstones().empty());
    EXPECT_EQ(table.getAllTombstones()[0], "apple");
}

// 12. Delete nonexistent key creates tombstone.
TEST_F(MemTableTest, DeleteNonexistentKeyCreatesTombstone) {
    MemTable table(TEST_WAL);

    EXPECT_FALSE(table.remove("missing"));

    auto deleted = table.getAllTombstones();

    ASSERT_EQ(deleted.size(), 1);
    EXPECT_EQ(deleted[0], "missing");
}

// 13. Put after delete removes tombstone.
TEST_F(MemTableTest, PutAfterDeleteRemovesTombstone) {
    MemTable table(TEST_WAL);

    ASSERT_TRUE(table.put("apple", "red"));
    ASSERT_TRUE(table.remove("apple"));
    ASSERT_TRUE(table.put("apple", "green"));

    std::string value;

    EXPECT_TRUE(table.get("apple", value));
    EXPECT_EQ(value, "green");
    EXPECT_TRUE(table.getAllTombstones().empty());
}

// 14. Tombstone survives restart.
TEST_F(MemTableTest, TombstoneSurvivesRestart) {
    {
        MemTable table(TEST_WAL);

        ASSERT_TRUE(table.put("apple", "red"));
        ASSERT_TRUE(table.remove("apple"));
    }

    {
        MemTable table(TEST_WAL);

        std::string value;

        EXPECT_FALSE(table.get("apple", value));

        auto deleted = table.getAllTombstones();

        ASSERT_EQ(deleted.size(), 1);
        EXPECT_EQ(deleted[0], "apple");
    }
}

// 15. Sequence number starts at zero.
TEST_F(MemTableSequenceTest, StartsAtSequenceZero) {
    MemTable memTable(walPath("test1.log"));

    EXPECT_EQ(memTable.getLastSequence(), 0);
    EXPECT_EQ(memTable.getNextSequence(), 1);
}

// 16. PUT increments sequence.
TEST_F(MemTableSequenceTest, PutIncrementsSequence) {
    MemTable memTable(walPath("test2.log"));

    ASSERT_TRUE(memTable.put("apple", "red"));

    EXPECT_EQ(memTable.getLastSequence(), 1);
    EXPECT_EQ(memTable.getNextSequence(), 2);
}

// 17. Multiple operations increment sequence.
TEST_F(MemTableSequenceTest, MultipleOperationsIncrementSequence) {
    MemTable memTable(walPath("test3.log"));

    ASSERT_TRUE(memTable.put("apple", "red"));
    ASSERT_TRUE(memTable.put("banana", "yellow"));
    ASSERT_TRUE(memTable.remove("apple"));

    EXPECT_EQ(memTable.getLastSequence(), 3);
}

// 18. Updating a key gives it a new sequence.
TEST_F(MemTableSequenceTest, UpdateGetsNewSequence) {
    MemTable memTable(walPath("test4.log"));

    ASSERT_TRUE(memTable.put("apple", "red"));
    ASSERT_TRUE(memTable.put("apple", "green"));

    auto records = memTable.getAllRecords();

    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].sequence, 2);
    EXPECT_EQ(records[0].value, "green");
}

// 19. Tombstone gets a sequence number.
TEST_F(MemTableSequenceTest, TombstoneGetsSequenceNumber) {
    MemTable memTable(walPath("test5.log"));

    ASSERT_TRUE(memTable.put("apple", "red"));
    ASSERT_TRUE(memTable.remove("apple"));

    auto records = memTable.getAllRecords();

    ASSERT_EQ(records.size(), 1);
    EXPECT_TRUE(records[0].isTombstone);
    EXPECT_EQ(records[0].sequence, 2);
}
