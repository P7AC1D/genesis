#include "genesis/core/Log.h"
#include <iostream>
#include <ctime>
#include <iomanip>

namespace Genesis {

    LogLevel Log::s_LogLevel = LogLevel::Trace;

    void Log::Init() {
        s_LogLevel = LogLevel::Trace;
    }

    void Log::Shutdown() {
        // Cleanup if needed
    }

    void Log::SetLevel(LogLevel level) {
        s_LogLevel = level;
    }

    void Log::Output(LogLevel level, const std::string& message) {
        // Get current time
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);

        // Color codes for terminal
        const char* colorCode = "\033[0m";
        const char* levelStr = "UNKNOWN";

        switch (level) {
            case LogLevel::Trace:
                colorCode = "\033[37m";  // White
                levelStr = "TRACE";
                break;
            case LogLevel::Debug:
                colorCode = "\033[36m";  // Cyan
                levelStr = "DEBUG";
                break;
            case LogLevel::Info:
                colorCode = "\033[32m";  // Green
                levelStr = "INFO";
                break;
            case LogLevel::Warn:
                colorCode = "\033[33m";  // Yellow
                levelStr = "WARN";
                break;
            case LogLevel::Error:
                colorCode = "\033[31m";  // Red
                levelStr = "ERROR";
                break;
            case LogLevel::Fatal:
                colorCode = "\033[35m";  // Magenta
                levelStr = "FATAL";
                break;
        }

        std::cout << colorCode << "["
                  << std::put_time(&tm, "%H:%M:%S") << "] "
                  << "[" << levelStr << "] "
                  << message
                  << "\033[0m" << std::endl;
    }

}
