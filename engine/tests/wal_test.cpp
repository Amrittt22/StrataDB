
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "WAL.h"
#include "MemTable.h"

namespace {

    const std::string TEST_WAL = "stratadb_wal_test.bin";
    const std::string TEST_MEMTABLE_WAL =
        "stratadb_wal_memtable_test.bin";

    void cleanWal() {
        std::filesystem::remove(TEST_WAL);
        std::filesystem::remove(TEST_MEMTABLE_WAL);
    }

}

// 1. Append and replay PUT operation
TEST(WALTest, AppendAndReplayPut) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        EXPECT_TRUE(wal.appendPut("name", "StrataDB"));

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        std::string value;

        EXPECT_TRUE(table.get("name", value));
        EXPECT_EQ(value, "StrataDB");
    }

    cleanWal();
}

// 2. Replay multiple operations
TEST(WALTest, ReplayMultipleOperations) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        EXPECT_TRUE(wal.appendPut("name", "StrataDB"));
        EXPECT_TRUE(wal.appendPut("language", "C++"));
        EXPECT_TRUE(wal.appendPut("database", "LSM Tree"));

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        std::string value;

        EXPECT_TRUE(table.get("name", value));
        EXPECT_EQ(value, "StrataDB");

        EXPECT_TRUE(table.get("language", value));
        EXPECT_EQ(value, "C++");

        EXPECT_TRUE(table.get("database", value));
        EXPECT_EQ(value, "LSM Tree");

        EXPECT_EQ(table.size(), 3);
    }

    cleanWal();
}

// 3. Replay updates to an existing key
TEST(WALTest, ReplayUpdatesExistingKey) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        EXPECT_TRUE(wal.appendPut("name", "OldValue"));
        EXPECT_TRUE(wal.appendPut("name", "NewValue"));

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        std::string value;

        EXPECT_TRUE(table.get("name", value));
        EXPECT_EQ(value, "NewValue");

        EXPECT_EQ(table.size(), 1);
    }

    cleanWal();
}

// 4. Replay DELETE operation
TEST(WALTest, ReplayDeleteOperation) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        EXPECT_TRUE(wal.appendPut("name", "StrataDB"));
        EXPECT_TRUE(wal.appendDelete("name"));

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        std::string value;

        EXPECT_FALSE(table.get("name", value));
        EXPECT_EQ(table.size(), 0);
    }

    cleanWal();
}

// 5. Handle binary keys and values
TEST(WALTest, HandlesBinaryKeyAndValue) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        std::string key("user\0id", 7);
        std::string value("hello\nworld\0!", 13);

        EXPECT_TRUE(wal.appendPut(key, value));

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        std::string retrievedValue;

        EXPECT_TRUE(table.get(key, retrievedValue));
        EXPECT_EQ(retrievedValue, value);
    }

    cleanWal();
}

// 6. Detect truncated WAL record

TEST(WALTest, RecoversFromTruncatedTail) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        // Write one valid record.
        ASSERT_TRUE(wal.appendPut("name", "StrataDB"));

        const auto validSize =
            std::filesystem::file_size(TEST_WAL);

        // Append an incomplete record.
        {
            std::ofstream out(
                TEST_WAL,
                std::ios::binary | std::ios::app
            );

            ASSERT_TRUE(out.is_open());

            // Operation byte without the remaining header.
            out.put(static_cast<char>(1));
        }

        MemTable table(TEST_MEMTABLE_WAL);

        // Recovery should succeed and discard the incomplete tail.
        EXPECT_TRUE(wal.replay(table));

        std::string value;

        // Previous valid record must survive.
        EXPECT_TRUE(table.get("name", value));
        EXPECT_EQ(value, "StrataDB");

        // WAL should be truncated to its valid size.
        EXPECT_EQ(
            std::filesystem::file_size(TEST_WAL),
            validSize
        );
    }

    cleanWal();
}


// 7. Missing WAL file should be treated as empty log
TEST(WALTest, MissingWalFileIsEmptyLog) {

    cleanWal();

    {
        WAL wal(TEST_WAL);

        MemTable table(TEST_MEMTABLE_WAL);

        EXPECT_TRUE(wal.replay(table));

        EXPECT_TRUE(table.empty());
        EXPECT_EQ(table.size(), 0);
    }

    cleanWal();
}
TEST(WALTest, SequenceNumbersSurviveReplay) {

    cleanWal();

    {
        MemTable table(TEST_MEMTABLE_WAL);

        ASSERT_TRUE(table.put("name", "old"));
        ASSERT_TRUE(table.put("name", "new"));
        ASSERT_TRUE(table.remove("name"));

        EXPECT_EQ(table.getLastSequence(), 3u);
    }

    {
        MemTable recovered(TEST_MEMTABLE_WAL);

        EXPECT_EQ(recovered.getLastSequence(), 3u);
        EXPECT_EQ(recovered.getNextSequence(), 4u);

        EXPECT_TRUE(recovered.hasTombstone("name"));
    }

    cleanWal();
}

TEST(WALTest, NewWritesContinueAfterRecovery) {

    cleanWal();

    {
        MemTable table(TEST_MEMTABLE_WAL);

        ASSERT_TRUE(table.put("a", "one"));
        ASSERT_TRUE(table.put("b", "two"));
    }

    {
        MemTable recovered(TEST_MEMTABLE_WAL);

        EXPECT_EQ(recovered.getNextSequence(), 3u);

        ASSERT_TRUE(recovered.put("c", "three"));

        EXPECT_EQ(recovered.getLastSequence(), 3u);
    }

    cleanWal();
}