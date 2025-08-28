#pragma once

#include "LoggerAsyncBase.h"
#include "utils/AnsiColors.h"

namespace vacdm::log {
class ConsoleLogger : public LoggerAsyncBase {
   private:
    static constexpr std::string_view logLevelToColor(const LogLevel level);
    std::ostream& m_out;

   protected:
    void emitLog(const LoggerAsyncBase::LogMessage& logMsg) override;

   public:
    ConsoleLogger(std::ostream& out = std::cout);
    // ConsoleLogger();
    ~ConsoleLogger() override;
};

}  // namespace vacdm::log