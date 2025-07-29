#pragma once

#include <Windows.h>
#include <shlwapi.h>

#include <filesystem>
#include <string>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace vacdm::utils {

class FileHelper {
   public:
    FileHelper() = delete;
    FileHelper(const FileHelper &) = delete;
    FileHelper(FileHelper &&) = delete;
    FileHelper &operator=(const FileHelper &) = delete;
    FileHelper &operator=(FileHelper &&) = delete;

    static std::string GetDllDirectoryPath() { return std::string{GetDllDirectoryPathCStr()}; }

    static std::filesystem::path GetDllDirectoryPathFs() { return std::filesystem::path{GetDllDirectoryPathCStr()}; }

   private:
    static const char *GetDllDirectoryPathCStr() {
        static char path[MAX_PATH + 1] = {0};
        GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
        PathRemoveFileSpecA(path);
        return path;
    }
};
}  // namespace vacdm::utils