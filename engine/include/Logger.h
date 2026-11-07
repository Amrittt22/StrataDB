#ifndef STRATADB_LOGGER_H
#define STRATADB_LOGGER_H

#include <string>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static void log(LogLevel level, const std::string& message);
};

#endif