#include "Logger.h"
#include <iostream>

void Logger::log(LogLevel level, const std::string& message) {
    std::string prefix;

    switch (level) {
        case LogLevel::INFO:
            prefix = "[INFO]";
            break;

        case LogLevel::WARNING:
            prefix = "[WARNING]";
            break;

        case LogLevel::ERROR:
            prefix = "[ERROR]";
            break;
    }

    std::cout << prefix << " " << message << std::endl;
}