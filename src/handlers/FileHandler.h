#pragma once

#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "log/Logger.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

using namespace vacdm::logging;

namespace vacdm::handlers {
/// @brief Utility class for handling file-related operations.
class FileHandler {
   private:
    static inline std::string m_dllPath;
    static inline std::string m_appDataPath;
    static inline bool initialised = false;

    static void checkInitialised() {
        if (true == initialised) return;

        // set DLL path
        char dllPath[MAX_PATH + 1] = {0};
        GetModuleFileNameA((HINSTANCE)&__ImageBase, dllPath, MAX_PATH);
        PathRemoveFileSpecA(dllPath);
        m_dllPath = std::string(dllPath);

        // set AppData folder path
        char appDataPath[MAX_PATH + 1] = {0};
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
            m_appDataPath = std::string(appDataPath);
        } else {
            Logger::instance().log(Logger::LogSender::FileHandler, "Could not set AppData path",
                                   Logger::LogLevel::Critical);
        }

        initialised = true;
    }

   public:
    static const std::string getDllPath() {
        checkInitialised();
        return m_dllPath;
    }

    static const std::string getAppDataPath() {
        checkInitialised();
        return m_appDataPath;
    }

    static void saveFile(const std::string& content, const std::string& filePath) {
        std::filesystem::path path(filePath);
        std::filesystem::path dir = path.parent_path();

        // Create directories if they don't exist
        std::error_code ec;
        if (!std::filesystem::exists(dir)) {
            if (!std::filesystem::create_directories(dir, ec) && ec) {
                Logger::instance().log(Logger::LogSender::FileHandler, "Failed to create directories: " + dir.string(),
                                       Logger::LogLevel::Error);
                return;
            }
        }

        std::ofstream outputFile(filePath);

        if (!outputFile.is_open()) {
            Logger::instance().log(Logger::LogSender::FileHandler, "Failed to open file for writing",
                                   Logger::LogLevel::Error);
            return;
        }

        outputFile << content;

        outputFile.close();

        Logger::instance().log(Logger::LogSender::FileHandler, "File saved successfully: " + filePath,
                               Logger::LogLevel::Info);
    }

    enum class FileState { FileExists, FileNotFound };

    static FileState checkFileExist(const std::string& fullPath) {
        if (std::filesystem::exists(fullPath))
            return FileState::FileExists;
        else
            return FileState::FileNotFound;
    }

    static void createFile(const std::string& fullPath) {
        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        std::ofstream createFile(fullPath);
        createFile.close();
    }
};
}  // namespace vacdm::handlers
