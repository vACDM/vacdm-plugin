#pragma once

#include "sqlite3.h"

namespace vacdm {
namespace logging {
    class Logger {
    public:
        enum class Level {
            Debug,
            Info,
            Warning,
            Error,
            Critical,
            System,
            Disabled
        };

    private:
        sqlite3* m_database;
        Level    m_minimumLevel;
        std::stringstream stream;
        bool logFileCreated = false;
        void createLogFile();
        Logger();
    public:
        ~Logger();
        void setMinimumLevel(Level level);
        void log(const std::string& component, Level level, const std::string& message);
        static Logger& instance();
    };
}
}
