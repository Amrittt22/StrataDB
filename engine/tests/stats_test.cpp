#include <gtest/gtest.h>
#include "Stats.h"

TEST(StatsTest, InitiallyAllCountsAreZero) {
    Stats stats;

    EXPECT_EQ(stats.getPutCount(), 0);
    EXPECT_EQ(stats.getGetCount(), 0);
    EXPECT_EQ(stats.getDeleteCount(), 0);
    EXPECT_EQ(stats.getSuccessfulGets(), 0);
    EXPECT_EQ(stats.getFailedGets(), 0);
    EXPECT_EQ(stats.getTotalOperations(), 0);
}

TEST(StatsTest, RecordsPutOperations) {
    Stats stats;

    stats.recordPut();
    stats.recordPut();

    EXPECT_EQ(stats.getPutCount(), 2);
    EXPECT_EQ(stats.getTotalOperations(), 2);
}

TEST(StatsTest, RecordsSuccessfulGets) {
    Stats stats;

    stats.recordGet(true);
    stats.recordGet(true);

    EXPECT_EQ(stats.getGetCount(), 2);
    EXPECT_EQ(stats.getSuccessfulGets(), 2);
    EXPECT_EQ(stats.getFailedGets(), 0);
}

TEST(StatsTest, RecordsFailedGets) {
    Stats stats;

    stats.recordGet(false);
    stats.recordGet(false);

    EXPECT_EQ(stats.getGetCount(), 2);
    EXPECT_EQ(stats.getSuccessfulGets(), 0);
    EXPECT_EQ(stats.getFailedGets(), 2);
}

TEST(StatsTest, RecordsMixedOperations) {
    Stats stats;

    stats.recordPut();
    stats.recordPut();
    stats.recordGet(true);
    stats.recordGet(false);
    stats.recordDelete();

    EXPECT_EQ(stats.getPutCount(), 2);
    EXPECT_EQ(stats.getGetCount(), 2);
    EXPECT_EQ(stats.getDeleteCount(), 1);
    EXPECT_EQ(stats.getSuccessfulGets(), 1);
    EXPECT_EQ(stats.getFailedGets(), 1);
    EXPECT_EQ(stats.getTotalOperations(), 5);
}