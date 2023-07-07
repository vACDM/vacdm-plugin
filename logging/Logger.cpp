#include <chrono>
#include <sstream>
#include <string>

#include "Logger.h"

using namespace vacdm::logging;

static const char __loggingTable[] = "CREATE TABLE messages( \
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP, \
    component TEXT, \
    level INT, \
    message TEXT \
);";
static const std::string __insertMessage = "INSERT INTO messages VALUES (CURRENT_TIMESTAMP, @1, @2, @3)";

Logger::Logger() :
        m_database(),
        m_minimumLevel(Logger::Level::Disabled) {
    stream << std::format("{0:%Y%m%d%H%M%S}", std::chrono::utc_clock::now()) << ".vacdm";
}

Logger::~Logger() {
    if (nullptr != this->m_database)
        sqlite3_close_v2(this->m_database);
}

void Logger::setMinimumLevel(Logger::Level level) {
    this->m_minimumLevel = level;
}

void Logger::log(const std::string& component, Logger::Level level, const std::string& message) {
    if (level < this->m_minimumLevel) {
        return;
    }
    
    if (logFileCreated == false) {
        createLogFile();
    }

    if (component.length() != 0 && message.length() != 0) {
        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(this->m_database, __insertMessage.c_str(), __insertMessage.length(), &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, component.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, static_cast<int>(level));
        sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_reset(stmt);
    }
}

void Logger::createLogFile() {
    sqlite3_open(stream.str().c_str(), &this->m_database);
    sqlite3_exec(this->m_database, __loggingTable, nullptr, nullptr, nullptr);
    sqlite3_exec(this->m_database, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr);
    logFileCreated = true;
}

Logger& Logger::instance() {
    static Logger __instance;
    return __instance;
}
