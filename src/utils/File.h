#pragma once

#include <Windows.h>
#include <shlwapi.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>

#include "log/ILogger.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace utils::file {

inline const char* GetDllDirectoryPathCStr() {
    static char path[MAX_PATH + 1] = {0};
    GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    return path;
}

inline std::string GetDllDirectoryPath() { return std::string{GetDllDirectoryPathCStr()}; }

inline std::filesystem::path GetDllDirectoryPathFs() { return std::filesystem::path{GetDllDirectoryPathCStr()}; }

}  // namespace utils::file
