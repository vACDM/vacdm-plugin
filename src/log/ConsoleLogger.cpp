
#include "ConsoleLogger.h"

#include <Windows.h>

#include <iostream>
#include <stdexcept>

using namespace vacdm::log;
using namespace vacdm::utils;

ConsoleLogger::ConsoleLogger() {
    if (GetConsoleWindow() == NULL) {
        if (!AllocConsole()) {
            DWORD errorCode = GetLastError();
            throw std::runtime_error("AllocConsole failed with error code: " + std::to_string(errorCode));
        }
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
};

ConsoleLogger::~ConsoleLogger() {};

inline constexpr std::string_view ConsoleLogger::logLevelToColor(const LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return colors::MAGENTA;
        case LogLevel::Info:
            return colors::WHITE;
        case LogLevel::Warning:
            return colors::YELLOW;
        case LogLevel::Error:
            return colors::RED;
        default:
            return colors::RESET;
    }
}

void ConsoleLogger::emitLog(const LoggerAsyncBase::LogMessage& logMsg) {
    std::time_t now_c = std::chrono::system_clock::to_time_t(logMsg.timestamp);
    const auto levelStr = logLevelToString(logMsg.level);
    const auto color = logLevelToColor(logMsg.level);

    std::string_view filename = logMsg.location.file_name();
    constexpr std::string_view prefix = "src\\";
    auto pos = filename.find(prefix);
    if (pos != std::string_view::npos) {
        filename.remove_prefix(pos + prefix.size());
    }

    std::cout << color << "[" << std::put_time(std::localtime(&now_c), "%F %T") << "] " << levelStr << " (" << filename
              << ":" << logMsg.location.line() << ") - " << logMsg.message << "\033[0m" << std::endl;
}
