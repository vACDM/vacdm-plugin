#pragma once

#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

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
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, dllPath))) {
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
};
}  // namespace vacdm::handlers
