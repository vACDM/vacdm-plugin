#pragma once

#include <sqlite3.h>

#include <filesystem>
#include <format>

#include "LoggerAsyncBase.h"

namespace vacdm::log {
class SqlLiteLogger : public LoggerAsyncBase {
   protected:
    void emitLog(const LoggerAsyncBase::LogMessage& logMsg) override;

   public:
    SqlLiteLogger();
    SqlLiteLogger(const std::filesystem::path& dirPath);
    ~SqlLiteLogger();

   private:
    sqlite3* m_database;

    void createLogFile(const std::filesystem::path& file);
    static std::string generateLogFileName() {
        return std::format("{:%Y%m%d%H%M%S}.vacdm", std::chrono::utc_clock::now());
    }
};
}  // namespace vacdm::log