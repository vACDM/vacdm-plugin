#pragma once

#include "LoggerAsyncBase.h"
#include "utils/AnsiColors.h"

namespace vacdm::log {
class ConsoleLogger : public LoggerAsyncBase {
   private:
    static constexpr std::string_view logLevelToColor(const LogLevel level);

   protected:
    void emitLog(const LoggerAsyncBase::LogMessage& logMsg) override;

   public:
    ConsoleLogger();
    ~ConsoleLogger();
};

}  // namespace vacdm::log