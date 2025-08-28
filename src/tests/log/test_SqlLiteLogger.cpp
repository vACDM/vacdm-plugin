#include <gtest/gtest.h>
#include <sqlite3.h>

#include <filesystem>
#include <fstream>

#include "log/SqlLiteLogger.h"

using namespace vacdm::log;
using namespace std::chrono;
namespace fs = std::filesystem;

TEST(SqlLiteLoggerTest, WritesLogMessageToDatabase) {
    fs::path tempDir = fs::temp_directory_path() / "vacdm_test_logs";

    if (fs::exists(tempDir)) {
        fs::remove_all(tempDir);
    }
    fs::create_directories(tempDir);

    std::filesystem::path logFilePath;
    {
        SqlLiteLogger logger(tempDir);
        logger.error("Unit test message");

        // Find the newly created log file
        for (const auto& entry : fs::directory_iterator(tempDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".vacdm") {
                logFilePath = entry.path();
                break;
            }
        }

        ASSERT_FALSE(logFilePath.empty()) << "Log file was not created.";
    }

    // Open the SQLite DB and query the entry
    sqlite3* db = nullptr;
    ASSERT_EQ(sqlite3_open(logFilePath.string().c_str(), &db), SQLITE_OK);

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, "SELECT message FROM messages;", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string msg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        EXPECT_TRUE(false) << "DB message: " << msg << "\n";
    }
    sqlite3_finalize(stmt);

    const char* query = "SELECT message FROM messages WHERE message LIKE '%Unit test message%' LIMIT 1;";
    // sqlite3_stmt* stmt = nullptr;

    ASSERT_EQ(sqlite3_prepare_v2(db, query, -1, &stmt, nullptr), SQLITE_OK);
    int stepResult = sqlite3_step(stmt);
    ASSERT_EQ(stepResult, SQLITE_ROW) << "No matching log message found in the database";

    std::string message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    EXPECT_EQ(message, "Unit test message");

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    fs::remove_all(tempDir);
}
