#pragma once

#include <cstdint>
#include <string>

#include <types/SystemConfig.h>

namespace vacdm {
    /**
     * @brief Defines the base class for all file formats
     * @ingroup format
     *
     * The base class provides error information, etc.
     */
    class FileFormat {
    protected:
        std::uint32_t m_errorLine;    /**< Defines the line number if a parser detected an error */
        std::string   m_errorMessage; /**< Defines the descriptive error message */

        /**
         * @brief Resets the internal structures
         */
        void reset() noexcept;

    private:
        bool parseColor(const std::string& block, COLORREF& color, std::uint32_t line);

    public:
        /**
         * @brief Creates the default file format
         */
        FileFormat() noexcept;

        /**
         * @brief Checks if an error was found
         * @return True if an error was found, else false
         */
        bool errorFound() const noexcept;
        /**
         * @brief Returns the error line
         * @return The error line
         */
        std::uint32_t errorLine() const noexcept;
        /**
         * @brief Returns the error message
         * @return The error message
         */
        const std::string& errorMessage() const noexcept;

        bool parse(const std::string& filename, SystemConfig& config);
    };
}
