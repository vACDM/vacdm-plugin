#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include "log/ConsoleLogger.h"

using namespace vacdm::log;

namespace vacdm::log::tests {

// RAII to redirect std::cout to a stringstream
struct CoutRedirect {
    std::streambuf* old;
    CoutRedirect(std::ostream& os, std::stringstream& ss) : old(os.rdbuf(ss.rdbuf())) {}
    ~CoutRedirect() { std::cout.rdbuf(old); }
};

TEST(ConsoleLoggerTest, LogDebugMessageFormatsCorrectly) {
    std::stringstream buffer;
    CoutRedirect redirect(std::cout, buffer);

    {
        ConsoleLogger logger;
        logger.log(LogLevel::Debug, "Test debug log");
    }

    std::string output = buffer.str();

    ASSERT_NE(output.find("DEBUG"), std::string::npos);
    ASSERT_NE(output.find("Test debug log"), std::string::npos);
    ASSERT_EQ(output.find("src/"), std::string::npos);
}

TEST(ConsoleLoggerTest, HighFrequency) {
    std::stringstream buffer;
    CoutRedirect redirect(std::cout, buffer);

    {
        ConsoleLogger logger;

        for (int i = 0; i < 1000; ++i) {
            logger.log(vacdm::log::LogLevel::Debug, "High frequency message #" + std::to_string(i));
        }
    }

    ASSERT_TRUE(buffer.str().find("High frequency message #0") != std::string::npos);
    ASSERT_TRUE(buffer.str().find("High frequency message #999") != std::string::npos);
}

}  // namespace vacdm::log::tests