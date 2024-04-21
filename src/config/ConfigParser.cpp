#include "ConfigParser.h"

#include <array>
#include <fstream>
#include <limits>
#include <vector>

#include "core/DataManager.h"
#include "handlers/FileHandler.h"
#include "utils/String.h"

using namespace vacdm;

ConfigParser::ConfigParser() : m_errorLine(std::numeric_limits<std::uint32_t>::max()), m_errorMessage() {}

bool ConfigParser::errorFound() const { return std::numeric_limits<std::uint32_t>::max() != this->m_errorLine; }

std::uint32_t ConfigParser::errorLine() const { return this->m_errorLine; }

const std::string &ConfigParser::errorMessage() const { return this->m_errorMessage; }

bool ConfigParser::parseColor(const std::string &block, COLORREF &color, std::uint32_t line) {
    std::vector<std::string> colorValues = utils::String::splitString(block, ",");

    if (3 != colorValues.size()) {
        this->m_errorLine = line;
        this->m_errorMessage = "Invalid color config";
        return false;
    }

    std::array<std::uint8_t, 3> colors;
    for (std::size_t i = 0; i < 3; ++i) colors[i] = static_cast<std::uint8_t>(std::atoi(colorValues[i].c_str()));

    color = RGB(colors[0], colors[1], colors[2]);

    return true;
}

bool ConfigParser::parse(PluginConfig &config) {
    config.valid = true;

    std::ifstream stream(handlers::FileHandler::getDllPath() + this->m_configFileName);
    if (stream.is_open() == false) {
        this->m_errorMessage = "Unable to open the configuration file";
        this->m_errorLine = 0;
        return false;
    }

    std::string line;
    std::uint32_t lineOffset = 0;
    while (std::getline(stream, line)) {
        std::string value;

        lineOffset += 1;

        /* skip empty lines */
        if (line.length() == 0) continue;

        /* trim the line and skip comments*/
        std::string trimmed = utils::String::trim(line);
        if (trimmed.find_first_of('#', 0) == 0) continue;

        // get values of line
        std::vector<std::string> values = utils::String::splitString(trimmed, "=");
        if (values.size() != 2) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid configuration entry";
            return false;
        } else {
            value = values[1];
        }

        /* end on invalid lines */
        if (0 == value.length()) {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Invalid entry";
            return false;
        }

        bool parsed = false;
        if ("SERVER_url" == values[0]) {
            config.serverUrl = values[1];
            parsed = true;
        } else if ("UPDATE_RATE_SECONDS" == values[0]) {
            try {
                const int updateCycleSeconds = std::stoi(values[1]);
                if (updateCycleSeconds < core::minUpdateCycleSeconds ||
                    updateCycleSeconds > core::maxUpdateCycleSeconds) {
                    this->m_errorLine = lineOffset;
                    this->m_errorMessage = "Value must be number between " +
                                           std::to_string(core::minUpdateCycleSeconds) + " and " +
                                           std::to_string(core::maxUpdateCycleSeconds);
                } else {
                    config.updateCycleSeconds = updateCycleSeconds;
                    parsed = true;
                }
            } catch (const std::exception &e) {
                this->m_errorMessage = e.what();
                this->m_errorLine = lineOffset;
            }

        } else if ("COLOR_lightgreen" == values[0]) {
            parsed = this->parseColor(values[1], config.lightgreen, lineOffset);
        } else if ("COLOR_lightblue" == values[0]) {
            parsed = this->parseColor(values[1], config.lightblue, lineOffset);
        } else if ("COLOR_green" == values[0]) {
            parsed = this->parseColor(values[1], config.green, lineOffset);
        } else if ("COLOR_blue" == values[0]) {
            parsed = this->parseColor(values[1], config.blue, lineOffset);
        } else if ("COLOR_lightyellow" == values[0]) {
            parsed = this->parseColor(values[1], config.lightyellow, lineOffset);
        } else if ("COLOR_yellow" == values[0]) {
            parsed = this->parseColor(values[1], config.yellow, lineOffset);
        } else if ("COLOR_orange" == values[0]) {
            parsed = this->parseColor(values[1], config.orange, lineOffset);
        } else if ("COLOR_red" == values[0]) {
            parsed = this->parseColor(values[1], config.red, lineOffset);
        } else if ("COLOR_grey" == values[0]) {
            parsed = this->parseColor(values[1], config.grey, lineOffset);
        } else if ("COLOR_white" == values[0]) {
            parsed = this->parseColor(values[1], config.white, lineOffset);
        } else if ("COLOR_debug" == values[0]) {
            parsed = this->parseColor(values[1], config.debug, lineOffset);
        } else {
            this->m_errorLine = lineOffset;
            this->m_errorMessage = "Unknown file entry: " + value[0];
            return false;
        }

        if (false == parsed) {
            return false;
        }
    }

    config.valid = true;
    return true;
}