#include "Logger.h"

#ifdef DEBUG_BUILD
#include <Windows.h>

#include <iostream>
#endif

#include <algorithm>
#include <chrono>
#include <numeric>

using namespace std::chrono_literals;
using namespace vacdm::logging;

static const char __loggingTable[] =
    "CREATE TABLE messages( \
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP, \
    sender TEXT, \
    level INT, \
    message TEXT \
);";
static const std::string __insertMessage = "INSERT INTO messages VALUES (CURRENT_TIMESTAMP, @1, @2, @3)";

Logger::Logger() {
    stream << std::format("{0:%Y%m%d%H%M%S}", std::chrono::utc_clock::now()) << ".vacdm";
#ifdef DEBUG_BUILD
    AllocConsole();
#pragma warning(push)
#pragma warning(disable : 6031)
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#pragma warning(pop)
    this->enableLogging();
#endif
    this->m_logWriter = std::thread(&Logger::run, this);
}

Logger::~Logger() {
    this->m_stop = true;
    this->m_logWriter.join();

    if (nullptr != this->m_database) sqlite3_close_v2(this->m_database);
}

void Logger::run() {
    while (true) {
        if (m_stop) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // obtain a copy of the logs, clear the log list to minimize lock time
        this->m_logLock.lock();
        auto logs = m_asynchronousLogs;
        m_asynchronousLogs.clear();
        this->m_logLock.unlock();

        auto it = logs.begin();
        while (it != logs.end()) {
            auto logsetting = std::find_if(logSettings.begin(), logSettings.end(),
                                           [it](const LogSetting &setting) { return setting.sender == it->sender; });

            if (logsetting != logSettings.end() && it->loglevel >= logsetting->minimumLevel) {
#ifdef DEBUG_BUILD
                std::cout << logsetting->name << ": " << it->message << "\n";
#endif

                sqlite3_stmt *stmt;

                sqlite3_prepare_v2(this->m_database, __insertMessage.c_str(), __insertMessage.length(), &stmt, nullptr);
                sqlite3_bind_text(stmt, 1, logsetting->name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 2, static_cast<int>(it->loglevel));
                sqlite3_bind_text(stmt, 3, it->message.c_str(), -1, SQLITE_TRANSIENT);

                sqlite3_step(stmt);
                sqlite3_clear_bindings(stmt);
                sqlite3_reset(stmt);
            }
            it = logs.erase(it);
        }
    }
}

void Logger::log(const LogSender &sender, const std::string &message, const LogLevel loglevel) {
    std::lock_guard guard(this->m_logLock);
    if (true == this->loggingEnabled) m_asynchronousLogs.push_back({sender, message, loglevel});
}

std::string Logger::handleLogLevelCommand(std::string sender, std::string newLevel) {
    std::lock_guard guard(this->m_logLock);
    auto logsetting = std::find_if(logSettings.begin(), logSettings.end(), [sender](const LogSetting &setting) {
        std::string uppercaseName = setting.name;
#pragma warning(push)
#pragma warning(disable : 4244)
        std::transform(uppercaseName.begin(), uppercaseName.end(), uppercaseName.begin(), ::toupper);
#pragma warning(pop)
        return uppercaseName == sender;
    });

    // sender not found
    if (logsetting == logSettings.end()) {
        return "Sender " + sender + " not found. Available senders are " +
               std::accumulate(std::next(logSettings.begin()), logSettings.end(), logSettings.front().name,
                               [](std::string acc, const LogSetting &setting) { return acc + " " + setting.name; });
    }

    // Modify logsetting by reference
    auto &logSettingRef = *logsetting;

#pragma warning(push)
#pragma warning(disable : 4244)
    std::transform(newLevel.begin(), newLevel.end(), newLevel.begin(), ::toupper);
#pragma warning(pop)

    if (newLevel == "DEBUG") {
        logSettingRef.minimumLevel = LogLevel::Debug;
    } else if (newLevel == "INFO") {
        logSettingRef.minimumLevel = LogLevel::Info;
    } else if (newLevel == "WARNING") {
        logSettingRef.minimumLevel = LogLevel::Warning;
    } else if (newLevel == "ERROR") {
        logSettingRef.minimumLevel = LogLevel::Error;
    } else if (newLevel == "CRITICAL") {
        logSettingRef.minimumLevel = LogLevel::Critical;
    } else if (newLevel == "SYSTEM") {
        logSettingRef.minimumLevel = LogLevel::System;
    } else if (newLevel == "DISABLED") {
        logSettingRef.minimumLevel = LogLevel::Disabled;
    } else {
        return "Invalid log level: " + newLevel;
    }

    // check if at least one sender is set to log
    bool enableLogging = false;
    for (auto logSetting : logSettings) {
        if (logsetting->minimumLevel != LogLevel::Disabled) {
            enableLogging = true;
            break;
        }
    }
    this->loggingEnabled = enableLogging;

    return "Changed sender " + sender + " to " + newLevel;
}

void Logger::enableLogging() {
    if (false == this->logFileCreated) createLogFile();

    std::lock_guard guard(this->m_logLock);
    this->loggingEnabled = true;
}

void Logger::disableLogging() {
    std::lock_guard guard(this->m_logLock);
    this->loggingEnabled = false;
}

void Logger::createLogFile() {
    sqlite3_open(stream.str().c_str(), &this->m_database);
    sqlite3_exec(this->m_database, __loggingTable, nullptr, nullptr, nullptr);
    sqlite3_exec(this->m_database, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr);
    logFileCreated = true;
}

Logger &Logger::instance() {
    static Logger __instance;
    return __instance;
}
