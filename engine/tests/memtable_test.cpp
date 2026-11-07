
#include <gtest/gtest.h>
#include "MemTable.h"

TEST(MemTableTest, InsertAndRetrieve) {
    MemTable table;

    table.put("name", "StrataDB");

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "StrataDB");
}

TEST(MemTableTest, UpdateExistingKey) {
    MemTable table;

    table.put("name", "OldValue");
    table.put("name", "NewValue");

    std::string value;

    EXPECT_TRUE(table.get("name", value));
    EXPECT_EQ(value, "NewValue");
    EXPECT_EQ(table.size(), 1);
}

TEST(MemTableTest, RetrieveMissingKey) {
    MemTable table;

    std::string value;

    EXPECT_FALSE(table.get("missing", value));
}

TEST(MemTableTest, DeleteExistingKey) {
    MemTable table;

    table.put("name", "StrataDB");

    EXPECT_TRUE(table.remove("name"));
    EXPECT_EQ(table.size(), 0);

    std::string value;
    EXPECT_FALSE(table.get("name", value));
}

TEST(MemTableTest, DeleteMissingKey) {
    MemTable table;

    EXPECT_FALSE(table.remove("missing"));
    EXPECT_EQ(table.size(), 0);
}

TEST(MemTableTest, MultipleEntries) {
    MemTable table;

    table.put("apple", "100");
    table.put("banana", "200");
    table.put("mango", "300");

    EXPECT_EQ(table.size(), 3);
    EXPECT_FALSE(table.empty());
}

TEST(MemTableTest, EmptyInitially) {
    MemTable table;

    EXPECT_TRUE(table.empty());
    EXPECT_EQ(table.size(), 0);
}
