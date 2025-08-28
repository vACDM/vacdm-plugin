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

enum class SaveFileStatus { Ok, MissingFilename, MissingExtension, CannotCreateDirectory, CannotOpenFile };

inline SaveFileStatus saveFile(const std::string& content, const std::filesystem::path& filePath,
                               const std::shared_ptr<vacdm::log::ILogger>& logger = nullptr) {
    // Must have a filename + extension
    if (!filePath.has_filename()) {
        return SaveFileStatus::MissingFilename;
    }
    if (!filePath.has_extension()) {
        if (logger) {
            logger->error("Missing file extension: " + filePath.string());
        }
        return SaveFileStatus::MissingExtension;
    }

    // Ensure directory exists
    auto dir = filePath.parent_path();
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            if (logger) {
                logger->error("Could not create directory: " + dir.string());
            }
            return SaveFileStatus::CannotCreateDirectory;
        }
    }

    // Try saving file
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) {
        if (logger) {
            logger->error("Could not open file: " + filePath.string());
        }
        return SaveFileStatus::CannotOpenFile;
    }

    out << content;
    out.close();

    return SaveFileStatus::Ok;
}

}  // namespace utils::file
