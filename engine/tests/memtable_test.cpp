
#include <gtest/gtest.h>
#include <filesystem>
#include <string>

#include "MemTable.h"

namespace {
    const std::string TEST_WAL = "memtable_test.wal";

    void cleanWal() {
        std::filesystem::remove(TEST_WAL);
    }
}

class MemTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        cleanWal();
    }

    void TearDown() override {
        cleanWal();
    }
};

// 1. Insert and retrieve
TEST_F(MemTableTest, InsertAndRetrieve) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "Amritanshu"));

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "Amritanshu");
}

// 2. Update existing key
TEST_F(MemTableTest, UpdateExistingKey) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "OldValue"));
    EXPECT_TRUE(table.put("name", "NewValue"));

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "NewValue");

    EXPECT_EQ(table.size(), 1);
}

// 3. Retrieve missing key
TEST_F(MemTableTest, RetrieveMissingKey) {
    MemTable table(TEST_WAL);

    std::string value;

    EXPECT_FALSE(table.get("missing", value));
}

// 4. Delete existing key
TEST_F(MemTableTest, DeleteExistingKey) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.put("name", "StrataDB"));
    EXPECT_TRUE(table.remove("name"));

    std::string value;

    EXPECT_FALSE(table.get("name", value));
    EXPECT_EQ(table.size(), 0);
}

// 5. Delete missing key
TEST_F(MemTableTest, DeleteMissingKey) {
    MemTable table(TEST_WAL);

    EXPECT_FALSE(table.remove("missing"));
    EXPECT_EQ(table.size(), 0);
}

// 6. Multiple entries
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

// 7. Initially empty
TEST_F(MemTableTest, EmptyInitially) {
    MemTable table(TEST_WAL);

    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);
}

// 8. WAL persistence after reopening MemTable
TEST_F(MemTableTest, RecoversDataAfterRestart) {

    // First instance writes data.
    {
        MemTable table(TEST_WAL);

        EXPECT_TRUE(table.put("name", "StrataDB"));
        EXPECT_TRUE(table.put("language", "C++"));

        EXPECT_EQ(table.size(), 2);
    }

    // Second instance recovers data from the same WAL.
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

// 9. WAL recovery preserves updates
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

// 10. WAL recovery preserves deletions
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
