#include<string>
#include <gtest/gtest.h>

// First StrataDB test
TEST(StrataDBTest, BasicTest) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(StrataDBTest, ProjectNameTest) {
    std::string projectName = "StrataDB";

    EXPECT_EQ(projectName, "StrataDB");
}
