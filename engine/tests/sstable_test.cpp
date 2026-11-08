#include <gtest/gtest.h>

#include "SSTable.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

class SSTableTest : public ::testing::Test {
protected:
    const std::string testFile = "test_sstable.dat";

    void TearDown() override {
        std::remove(testFile.c_str());
    }
};

TEST_F(SSTableTest, WritesAndLoadsEntries) {
    const std::vector<std::pair<std::string, std::string>> data = {
        {"banana", "yellow"}, {"apple", "red"}, {"cherry", "red"}};
    ASSERT_TRUE(SSTable::write(testFile, data));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    EXPECT_EQ(table.size(), 3);
}

TEST_F(SSTableTest, RetrievesExistingKey) {
    const std::vector<std::pair<std::string, std::string>> data = {
        {"apple", "red"}, {"banana", "yellow"}};
    ASSERT_TRUE(SSTable::write(testFile, data));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    std::string value;
    EXPECT_TRUE(table.get("apple", value));
    EXPECT_EQ(value, "red");
}

TEST_F(SSTableTest, ReturnsFalseForMissingKey) {
    const std::vector<std::pair<std::string, std::string>> data = {
        {"apple", "red"}, {"banana", "yellow"}};
    ASSERT_TRUE(SSTable::write(testFile, data));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    std::string value;
    EXPECT_FALSE(table.get("orange", value));
}

TEST_F(SSTableTest, SortsEntriesBeforeWriting) {
    const std::vector<std::pair<std::string, std::string>> data = {
        {"zebra", "1"}, {"apple", "2"}, {"mango", "3"}};
    ASSERT_TRUE(SSTable::write(testFile, data));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    std::string value;
    EXPECT_TRUE(table.get("apple", value));
    EXPECT_EQ(value, "2");
    EXPECT_TRUE(table.get("mango", value));
    EXPECT_EQ(value, "3");
    EXPECT_TRUE(table.get("zebra", value));
    EXPECT_EQ(value, "1");
}

TEST_F(SSTableTest, HandlesEmptyTable) {
    ASSERT_TRUE(SSTable::write(testFile, {}));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    EXPECT_EQ(table.size(), 0);
}

TEST_F(SSTableTest, HandlesBinaryStrings) {
    const std::string key("key\0part", 8);
    const std::string value("value\0part", 10);
    ASSERT_TRUE(SSTable::write(testFile, {{key, value}}));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    std::string result;
    EXPECT_TRUE(table.get(key, result));
    EXPECT_EQ(result, value);
}

TEST_F(SSTableTest, RejectsDuplicateKeys) {
    EXPECT_FALSE(SSTable::write(
        testFile, {{"apple", "red"}, {"apple", "green"}}));
}

TEST_F(SSTableTest, RejectsInvalidFile) {
    {
        std::ofstream out(testFile, std::ios::binary);
        out << "NOT_A_VALID_SSTABLE";
    }
    SSTable table;
    EXPECT_FALSE(table.load(testFile));
    EXPECT_FALSE(table.isLoaded());
}

TEST_F(SSTableTest, BloomFilterContainsInsertedKeys) {
    ASSERT_TRUE(SSTable::write(
        testFile, {{"apple", "red"}, {"banana", "yellow"}, {"cherry", "red"}}));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    EXPECT_TRUE(table.mightContain("apple"));
    EXPECT_TRUE(table.mightContain("banana"));
    EXPECT_TRUE(table.mightContain("cherry"));
}

TEST_F(SSTableTest, DoesNotFindMissingKey) {
    ASSERT_TRUE(SSTable::write(testFile, {{"apple", "red"}}));
    SSTable table;
    ASSERT_TRUE(table.load(testFile));
    std::string value;
    EXPECT_FALSE(table.get("mango", value));
}
