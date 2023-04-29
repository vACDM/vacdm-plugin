#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>

#include "Logger.h"

#define PERFORMANCE_MEASUREMENT 1

namespace vacdm {
namespace logging {
    class Performance {
    private:
        std::string m_name;
        std::uint64_t m_average;
        std::int64_t m_minimum;
        std::int64_t m_maximum;
        std::uint32_t m_cycleCount;
        std::chrono::high_resolution_clock::time_point m_startTime;
        std::chrono::high_resolution_clock::time_point m_lastLogStamp;

    public:
        Performance(const std::string& name) :
            m_name(name),
            m_average(0),
            m_minimum(std::numeric_limits<std::uint32_t>::max()),
            m_maximum(std::numeric_limits<std::uint32_t>::min()),
            m_cycleCount(0),
            m_startTime(),
            m_lastLogStamp(std::chrono::high_resolution_clock::now()) {}

        void start() {
#if PERFORMANCE_MEASUREMENT
            this->m_startTime = std::chrono::high_resolution_clock::now();
#endif
        }

        void stop() {
#if PERFORMANCE_MEASUREMENT
            const auto now = std::chrono::high_resolution_clock::now();
            const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - this->m_startTime).count();

            this->m_minimum = std::min(this->m_minimum, delta);
            this->m_maximum = std::max(this->m_maximum, delta);
            if (this->m_cycleCount == 0)
                this->m_average = delta;
            else
                this->m_average = static_cast<std::uint32_t>((this->m_average + delta) * 0.5);

            if (std::chrono::duration_cast<std::chrono::minutes>(now - this->m_lastLogStamp).count() >= 1) {
                std::stringstream stream;
                stream << "Performance statistics:" << std::endl;
                stream << "    Average: " << std::to_string(this->m_average) << " us" << std::endl;
                stream << "    Minimum: " << std::to_string(this->m_minimum) << " us" << std::endl;
                stream << "    Maximum: " << std::to_string(this->m_maximum) << " us" << std::endl;

                Logger::instance().log("Performance-" + this->m_name, Logger::Level::Info, stream.str());

                this->m_lastLogStamp = now;
            }
#endif
        }
    };
}
}
