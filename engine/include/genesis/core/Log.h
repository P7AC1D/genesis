#pragma once

#include <string>
#include <memory>
#include <sstream>

namespace Genesis {

    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Fatal
    };

    class Log {
    public:
        static void Init();
        static void Shutdown();

        static void SetLevel(LogLevel level);

        template<typename... Args>
        static void Trace(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Trace, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Debug(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Debug, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Info(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Info, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Warn(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Warn, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Error(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Error, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Fatal(const char* fmt, Args&&... args) {
            LogMessage(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
        }

    private:
        template<typename... Args>
        static void LogMessage(LogLevel level, const char* fmt, Args&&... args) {
            if (level < s_LogLevel) return;
            
            std::ostringstream oss;
            FormatMessage(oss, fmt, std::forward<Args>(args)...);
            Output(level, oss.str());
        }

        static void FormatMessage(std::ostringstream& oss, const char* fmt) {
            oss << fmt;
        }

        template<typename T, typename... Args>
        static void FormatMessage(std::ostringstream& oss, const char* fmt, T&& value, Args&&... args) {
            while (*fmt) {
                if (*fmt == '{' && *(fmt + 1) == '}') {
                    oss << std::forward<T>(value);
                    FormatMessage(oss, fmt + 2, std::forward<Args>(args)...);
                    return;
                }
                oss << *fmt++;
            }
        }

        static void Output(LogLevel level, const std::string& message);

    private:
        static LogLevel s_LogLevel;
    };

}

// Convenience macros
#define GEN_TRACE(...) ::Genesis::Log::Trace(__VA_ARGS__)
#define GEN_DEBUG(...) ::Genesis::Log::Debug(__VA_ARGS__)
#define GEN_INFO(...)  ::Genesis::Log::Info(__VA_ARGS__)
#define GEN_WARN(...)  ::Genesis::Log::Warn(__VA_ARGS__)
#define GEN_ERROR(...) ::Genesis::Log::Error(__VA_ARGS__)
#define GEN_FATAL(...) ::Genesis::Log::Fatal(__VA_ARGS__)

#ifdef GENESIS_DEBUG
    #define GEN_ASSERT(condition, ...) \
        if (!(condition)) { \
            GEN_FATAL("Assertion failed: {}", #condition); \
            GEN_FATAL(__VA_ARGS__); \
            std::abort(); \
        }
#else
    #define GEN_ASSERT(condition, ...)
#endif
