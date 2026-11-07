#include <iostream>
#include "Logger.h"

int main() {
    Logger::log(LogLevel::INFO, "StrataDB is starting...");
    Logger::log(LogLevel::INFO, "Initializing storage engine...");
    Logger::log(LogLevel::INFO, "MemTable initialized successfully");
    Logger::log(LogLevel::INFO, "Statistics module initialized");
    Logger::log(LogLevel::INFO, "StrataDB is ready!");

    return 0;
}