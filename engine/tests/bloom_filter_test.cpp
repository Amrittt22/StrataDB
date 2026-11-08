#include <gtest/gtest.h>

#include "BloomFilter.h"

#include <stdexcept>
#include <string>

TEST(BloomFilterTest, InitiallyDoesNotContainInsertedNothing) {
    BloomFilter filter(100);
    EXPECT_FALSE(filter.mightContain("hello"));
}

TEST(BloomFilterTest, FindsInsertedKey) {
    BloomFilter filter(100);
    filter.add("apple");
    EXPECT_TRUE(filter.mightContain("apple"));
}

TEST(BloomFilterTest, FindsAllInsertedKeys) {
    BloomFilter filter(1000);
    for (int i = 0; i < 500; i++) {
        filter.add("key" + std::to_string(i));
    }
    for (int i = 0; i < 500; i++) {
        EXPECT_TRUE(filter.mightContain("key" + std::to_string(i)));
    }
}

TEST(BloomFilterTest, HandlesEmptyString) {
    BloomFilter filter(10);
    filter.add("");
    EXPECT_TRUE(filter.mightContain(""));
}

TEST(BloomFilterTest, RejectsInvalidFalsePositiveRate) {
    EXPECT_THROW(BloomFilter(100, 0.0), std::invalid_argument);
    EXPECT_THROW(BloomFilter(100, 1.0), std::invalid_argument);
}
