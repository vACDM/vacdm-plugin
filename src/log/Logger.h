#pragma once

#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "sqlite3.h"

namespace vacdm::logging {
class Logger {
   public:
    enum LogSender {
        vACDM,
        DataManager,
        Server,
        ConfigParser,
        Utils,
    };

    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        System,
        Disabled,
    };

    struct LogSetting {
        LogSender sender;
        std::string name;
        LogLevel minimumLevel;
    };

    struct AsynchronousLog {
        LogSender sender;
        std::string message;
        LogLevel loglevel;
    };

   private:
    Logger();
#ifdef DEBUG_BUILD
    std::vector<LogSetting> logSettings = {
        {vACDM, "vACDM", Debug},   {DataManager, "DataManager", Info},
        {Server, "Server", Debug}, {ConfigParser, "ConfigParser", Debug},
        {Utils, "Utils", Debug},
    };
#else
    /// @brief set the log level for each sender separately
    std::vector<LogSetting> logSettings = {
        {vACDM, "vACDM", Disabled},   {DataManager, "DataManager", Disabled},
        {Server, "Server", Disabled}, {ConfigParser, "ConfigParser", Disabled},
        {Utils, "Utils", Disabled},
    };
#endif
    bool m_LogAll = false;

    std::mutex m_logLock;
    std::list<struct AsynchronousLog> m_asynchronousLogs;
    std::thread m_logWriter;
    bool m_stop = false;
    void run();

    void enableLogging();
    void disableLogging();
    bool loggingEnabled = false;

    sqlite3 *m_database;
    std::stringstream stream;
    bool logFileCreated = false;
    void createLogFile();

   public:
    ~Logger();
    /// @brief queues a log message to be processed asynchronously
    /// @param sender the sender (e.g. class)
    /// @param message the message to be displayed
    /// @param loglevel the severity, must be greater than m_minimumLogLevel to be logged
    void log(const LogSender &sender, const std::string &message, const LogLevel loglevel);
    std::string handleLogCommand(std::string command);
    std::string handleLogLevelCommand(std::string command);
    static Logger &instance();
};
}  // namespace vacdm::logging