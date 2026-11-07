#include "gtest/gtest.h"
#include "SkipList.h"

TEST(SkipListTest, InsertAndSearch) {
    SkipList list;

    list.insert("apple", "100");

    std::string value;

    EXPECT_TRUE(list.search("apple", value));
    EXPECT_EQ(value, "100");
}

TEST(SkipListTest, SearchMissingKey) {
    SkipList list;

    std::string value;

    EXPECT_FALSE(list.search("banana", value));
}

TEST(SkipListTest, UpdateExistingKey) {
    SkipList list;

    list.insert("apple", "100");
    list.insert("apple", "200");

    std::string value;

    EXPECT_TRUE(list.search("apple", value));
    EXPECT_EQ(value, "200");
}

TEST(SkipListTest, DeleteExistingKey) {
    SkipList list;

    list.insert("apple", "100");

    EXPECT_TRUE(list.remove("apple"));

    std::string value;

    EXPECT_FALSE(list.search("apple", value));
}

TEST(SkipListTest, DeleteMissingKey) {
    SkipList list;

    EXPECT_FALSE(list.remove("banana"));
}

TEST(SkipListTest, MaintainsSortedSearchBehavior) {
    SkipList list;

    list.insert("mango", "400");
    list.insert("apple", "100");
    list.insert("banana", "200");

    std::string value;

    EXPECT_TRUE(list.search("apple", value));
    EXPECT_EQ(value, "100");

    EXPECT_TRUE(list.search("banana", value));
    EXPECT_EQ(value, "200");

    EXPECT_TRUE(list.search("mango", value));
    EXPECT_EQ(value, "400");
}


TEST(SkipListTest, EmptyListSearch) {
    SkipList list;
    std::string value;

    EXPECT_FALSE(list.search("apple", value));
}

TEST(SkipListTest, InsertMultipleKeys) {
    SkipList list;

    for (int i = 0; i < 1000; i++) {
        list.insert("key" + std::to_string(i),
                    "value" + std::to_string(i));
    }

    std::string value;

    EXPECT_TRUE(list.search("key500", value));
    EXPECT_EQ(value, "value500");
}

TEST(SkipListTest, DeleteMiddleKey) {
    SkipList list;

    list.insert("apple", "1");
    list.insert("banana", "2");
    list.insert("cherry", "3");

    EXPECT_TRUE(list.remove("banana"));

    std::string value;

    EXPECT_TRUE(list.search("apple", value));
    EXPECT_FALSE(list.search("banana", value));
    EXPECT_TRUE(list.search("cherry", value));
}

TEST(SkipListTest, DeleteAllKeys) {
    SkipList list;

    list.insert("apple", "1");
    list.insert("banana", "2");
    list.insert("cherry", "3");

    EXPECT_TRUE(list.remove("apple"));
    EXPECT_TRUE(list.remove("banana"));
    EXPECT_TRUE(list.remove("cherry"));

    std::string value;
    EXPECT_FALSE(list.search("apple", value));
}

