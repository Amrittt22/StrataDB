#include <gtest/gtest.h>
#include "Logger.h"
#include <sstream>
#include <iostream>

TEST(LoggerTest, LogsInfoMessage) {
    std::stringstream buffer;
    std::streambuf* oldBuffer = std::cout.rdbuf(buffer.rdbuf());

    Logger::log(LogLevel::INFO, "Database started");

    std::cout.rdbuf(oldBuffer);

    EXPECT_EQ(buffer.str(), "[INFO] Database started\n");
}

TEST(LoggerTest, LogsWarningMessage) {
    std::stringstream buffer;
    std::streambuf* oldBuffer = std::cout.rdbuf(buffer.rdbuf());

    Logger::log(LogLevel::WARNING, "Key not found");

    std::cout.rdbuf(oldBuffer);

    EXPECT_EQ(buffer.str(), "[WARNING] Key not found\n");
}

TEST(LoggerTest, LogsErrorMessage) {
    std::stringstream buffer;
    std::streambuf* oldBuffer = std::cout.rdbuf(buffer.rdbuf());

    Logger::log(LogLevel::ERROR, "Storage failure");

    std::cout.rdbuf(oldBuffer);

    EXPECT_EQ(buffer.str(), "[ERROR] Storage failure\n");
}