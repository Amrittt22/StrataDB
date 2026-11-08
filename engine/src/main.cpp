#include <iostream>
#include <string>
#include <exception>

#include "Logger.h"
#include "MemTable.h"

int main() {

    try {
        Logger::log(
            LogLevel::INFO,
            "StrataDB is starting..."
        );

        MemTable table("stratadb.wal");

        Logger::log(
            LogLevel::INFO,
            "WAL recovery completed"
        );

        Logger::log(
            LogLevel::INFO,
            "MemTable initialized successfully"
        );

        std::string value;

        if (table.put("name", "StrataDB")) {
            Logger::log(
                LogLevel::INFO,
                "PUT operation successful"
            );
        }

        if (table.get("name", value)) {
            Logger::log(
                LogLevel::INFO,
                "Retrieved value: " + value
            );
        }

        Logger::log(
            LogLevel::INFO,
            "StrataDB is ready!"
        );

    }
    catch (const std::exception& e) {
        Logger::log(
            LogLevel::ERROR,
            e.what()
        );

        return 1;
    }

    return 0;
}