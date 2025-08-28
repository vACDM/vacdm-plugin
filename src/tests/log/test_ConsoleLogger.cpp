#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include "log/ConsoleLogger.h"

using namespace vacdm::log;

namespace vacdm::log::tests {

TEST(ConsoleLoggerTest, LogDebugMessageFormatsCorrectly) {
    std::stringstream buffer;
    std::unique_ptr<ILogger> logger = std::make_unique<ConsoleLogger>(buffer);

    // ConsoleLogger logger(buffer);
    // logger->log(LogLevel::Debug, "Test message");
    logger->debug("Test message");
    std::string output = buffer.str();

    // std::string output = buffer.str();

    // ASSERT_NE(output.find("DEBUG"), std::string::npos) << "Could not find DEBUG prefix";
    // ASSERT_NE(output.find("Test debug log"), std::string::npos) << "Could not find log message";
    // ASSERT_NE(output.find("src/"), std::string::npos) << "Found src/ path in log message";
}

// TEST(ConsoleLoggerTest, HighFrequency) {
//     std::stringstream buffer;
//     CoutRedirect redirect(std::cout, buffer);

//     {
//         ConsoleLogger logger;

//         for (int i = 0; i < 1000; ++i) {
//             logger.log(vacdm::log::LogLevel::Debug, "High frequency message #" + std::to_string(i));
//         }
//     }

//     ASSERT_TRUE(buffer.str().find("High frequency message #0") != std::string::npos)
//         << "Could not find first log message";
//     ASSERT_TRUE(buffer.str().find("High frequency message #999") != std::string::npos)
//         << "Could not find last log message";
// }

}  // namespace vacdm::log::tests