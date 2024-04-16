#pragma once

#include <chrono>
#include <string>

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include "log/Logger.h"

namespace vacdm::utils {

class Date {
   public:
    Date() = delete;
    Date(const Date &) = delete;
    Date(Date &&) = delete;
    Date &operator=(const Date &) = delete;
    Date &operator=(Date &&) = delete;

    /// @brief Converts std::chrono::utc_clock::time_point to an ISO-formatted string.
    ///
    /// This function takes a std::chrono::utc_clock::time_point and converts it to an
    /// ISO-formatted string. The resulting string follows the format "%Y-%m-%dT%H:%M:%S.Z".
    /// The function ensures proper formatting, including the addition of "Z" to indicate UTC.
    ///
    /// If the provided time_point is non-negative (i.e., not before the epoch), the
    /// function formats it into a string and appends "Z" to indicate UTC. If the time_point
    /// is negative, the function returns a default timestamp representing "1969-12-31T23:59:59.999Z".
    ///
    /// @param timepoint std::chrono::utc_clock::time_point to be converted.
    /// @return ISO-formatted string representing the converted timestamp.
    static std::string timestampToIsoString(const std::chrono::utc_clock::time_point &timepoint) {
        if (timepoint.time_since_epoch().count() >= 0) {
            std::stringstream stream;
            stream << std::format("{0:%FT%T}", timepoint);
            auto timestamp = stream.str();
            timestamp = timestamp.substr(0, timestamp.length() - 4) + "Z";
            return timestamp;
        } else {
            return "1969-12-31T23:59:59.999Z";
        }
    }

    /// @brief Converts an ISO-formatted string to std::chrono::utc_clock::time_point.
    ///
    /// This function takes an ISO-formatted string representing a timestamp and converts
    /// it to a std::chrono::utc_clock::time_point. The input string is expected to be in
    /// the format "%Y-%m-%dT%H:%M:%S".
    ///
    /// The function uses a std::stringstream to parse the input string and create a
    /// std::chrono::utc_clock::time_point. The resulting time_point is then returned.
    ///
    /// @param timestamp ISO-formatted string representing the timestamp.
    /// @return std::chrono::utc_clock::time_point representing the converted timestamp.
    static std::chrono::utc_clock::time_point isoStringToTimestamp(const std::string &timestamp) {
        std::chrono::utc_clock::time_point retval;
        std::stringstream stream;

        stream << timestamp.substr(0, timestamp.length() - 1);
        std::chrono::from_stream(stream, "%FT%T", retval);

        return retval;
    }

    /// @brief Converts a EuroScope departure time string to a UTC time_point.
    /// This function takes a EuroScope flight plan and extracts the estimated departure time string.
    /// Using a different util function it then convert the string to a utc time_point
    ///
    /// @param flightplan EuroScope flight plan to extract information from.
    /// @return std::chrono::utc_clock::time_point representing the converted departure time.
    ///
    static std::chrono::utc_clock::time_point convertEuroscopeDepartureTime(
        const EuroScopePlugIn::CFlightPlan flightplan) {
        const std::string callsign = flightplan.GetCallsign();
        const std::string eobt = flightplan.GetFlightPlanData().GetEstimatedDepartureTime();

        return convertStringToTimePoint(eobt);
    }
    /// @brief Converts a 4-character HHMM string to a UTC time_point.
    /// This function takes a 4-character string representing time in HHMM format.
    /// If the string is not valid (empty or exceeds 4 characters), the function returns the
    /// current UTC time. Otherwise, it constructs a formatted string combining the current
    /// date (year, month, day) with the given time, ensuring proper zero-padding.
    /// The formatted string is then parsed to obtain a std::chrono::utc_clock::time_point.
    ///
    /// @param hhmmString The 4-character string representing time in HHMM format.
    /// @return std::chrono::utc_clock::time_point representing the converted time.
    ///
    static std::chrono::utc_clock::time_point convertStringToTimePoint(const std::string &hhmmString) {
        const auto now = std::chrono::utc_clock::now();
        if (hhmmString.length() == 0 || hhmmString.length() > 4) {
            return now;
        }

        std::stringstream stream;
        stream << std::format("{0:%Y%m%d}", now);
        std::size_t requiredLeadingZeros = 4 - hhmmString.length();
        while (requiredLeadingZeros != 0) {
            requiredLeadingZeros -= 1;
            stream << "0";
        }
        stream << hhmmString;

        std::chrono::utc_clock::time_point time;
        std::stringstream input(stream.str());
        std::chrono::from_stream(stream, "%Y%m%d%H%M", time);

        return time;
    }
};
}  // namespace vacdm::utils
