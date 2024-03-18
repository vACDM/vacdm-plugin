#include <array>
#include <limits>
#include <iostream>
#include <fstream>

#include <helper/String.h>

#include "FileFormat.h"

using namespace vacdm;

FileFormat::FileFormat() noexcept :
    m_errorLine(std::numeric_limits<std::uint32_t>::max()),
    m_errorMessage() { }

void FileFormat::reset() noexcept {
    this->m_errorLine = std::numeric_limits<std::uint32_t>::max();
    this->m_errorMessage.clear();
}

bool FileFormat::errorFound() const noexcept {
    return std::numeric_limits<std::uint32_t>::max() != this->m_errorLine;
}

std::uint32_t FileFormat::errorLine() const noexcept {
    return this->m_errorLine;
}

const std::string& FileFormat::errorMessage() const noexcept {
    return this->m_errorMessage;
}

bool FileFormat::parseColor(const std::string& block, COLORREF& color, std::uint32_t line) {
    auto split = helper::String::splitString(block, ",");
    if (3 != split.size()) {
        this->m_errorLine = line;
        this->m_errorMessage = "Invalid color configuration";
        return false;
    }

    std::array<std::uint8_t, 3> data;
    for (std::size_t i = 0; i < 3; ++i)
        data[i] = static_cast<std::uint8_t>(std::atoi(split[i].c_str()));
    color = RGB(data[0], data[1], data[2]);

    return true;
}

bool FileFormat::parse(const std::string& filename, SystemConfig& config) {
    config.valid = false;

    std::ifstream stream(filename);
    if (false == stream.is_open()) {
        this->m_errorMessage = "Unable to open the configuration file";
        this->m_errorLine = 0;
        return false;
    }

    std::string line;
    std::uint32_t lineOffset = 0;
    while (std::getline(stream, line)) {
        std::string value;

        lineOffset += 1;

        /* skip a new line */
        if (0 == line.length())
            continue;

        /* trimm the line and check if a comment line is used */
        std::string trimmed = helper::String::trim(line);
        if (0 == trimmed.find_first_of('#', 0))
            continue;

        auto entry = helper::String::splitString(trimmed, "=");
        if (2 > entry.size()) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid configuration entry";
            config.valid = false;
            return false;
        }
        else if (2 < entry.size()) {
            for (std::size_t idx = 1; idx < entry.size() - 1; ++idx)
                value += entry[idx] + "=";
            value += entry.back();
        }
        else {
            value = entry[1];
        }

        /* found an invalid line */
        if (0 == value.length()) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid entry";
            return false;
        }

        bool parsed = false;
        if ("SERVER_url" == entry[0]) {
            config.serverUrl = entry[1];
            parsed = true;
        } else if ("COLOR_lightgreen" == entry[0]) {
            parsed = this->parseColor(entry[1], config.lightgreen, lineOffset);
        } else if ("COLOR_lightblue" == entry[0]) {
            parsed = this->parseColor(entry[1], config.lightblue, lineOffset);
        } else if ("COLOR_green" == entry[0]) {
            parsed = this->parseColor(entry[1], config.green, lineOffset);
        } else if ("COLOR_blue" == entry[0]) {
            parsed = this->parseColor(entry[1], config.blue, lineOffset);
        } else if ("COLOR_lightyellow" == entry[0]) {
            parsed = this->parseColor(entry[1], config.lightyellow, lineOffset);
        } else if ("COLOR_yellow" == entry[0]) {
            parsed = this->parseColor(entry[1], config.yellow, lineOffset);
        } else if ("COLOR_orange" == entry[0]) {
            parsed = this->parseColor(entry[1], config.orange, lineOffset);
        } else if ("COLOR_red" == entry[0]) {
            parsed = this->parseColor(entry[1], config.red, lineOffset);
        } else if ("COLOR_grey" == entry[0]) {
            parsed = this->parseColor(entry[1], config.grey, lineOffset);
        } else if ("COLOR_white" == entry[0]) {
            parsed = this->parseColor(entry[1], config.white, lineOffset);
        } else if ("COLOR_debug" == entry[0]) {
            parsed = this->parseColor(entry[1], config.debug, lineOffset);
        } else {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Unknown file entry: " + entry[0];
            return false;
        }

        if (false == parsed) {
            return false;
        }
    }

    config.valid = true;
    return true;
}
