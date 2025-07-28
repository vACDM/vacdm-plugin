#pragma once

#include <source_location>
#include <string>

namespace vacdm::log {
enum class LogLevel { Debug, Info, Warning, Error, Disabled };

inline constexpr std::string_view logLevelToString(const LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Disabled:
            return "DISABLED";
        default:
            return "UNKNOWN";
    }
}

class ILogger {
   public:
    virtual ~ILogger() = default;

    virtual void log(const LogLevel level, const std::string& message,
                     const std::source_location location = std::source_location::current()) = 0;

    virtual void debug(const std::string& message,
                       const std::source_location location = std::source_location::current()) {
        log(LogLevel::Debug, message, location);
    }

    virtual void info(const std::string& message,
                      const std::source_location location = std::source_location::current()) {
        log(LogLevel::Info, message, location);
    }

    virtual void warn(const std::string& message,
                      const std::source_location location = std::source_location::current()) {
        log(LogLevel::Warning, message, location);
    }

    virtual void error(const std::string& message,
                       const std::source_location location = std::source_location::current()) {
        log(LogLevel::Error, message, location);
    }
};

}  // namespace vacdm::log
