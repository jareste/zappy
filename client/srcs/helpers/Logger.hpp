#pragma once

#include <string>
#include <mutex>

enum class LogLevel {
    Debug = 0,
    Info,
    Warn,
    Error
};

class Logger {
    private:
        static void log(LogLevel level, const std::string &message);
        static const char* levelToString(LogLevel level);
        static bool deepTraceEnabled();
    
    public:
        static void setLevel(LogLevel level);
        static void debug(const std::string &message);
        static void info(const std::string &message);
        static void warn(const std::string &message);
        static void error(const std::string &message);
        static bool isDeepTraceEnabled();
        static void trace(const std::string& category, const std::string& message);
};
