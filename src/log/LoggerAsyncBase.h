#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <source_location>
#include <string>
#include <thread>

#include "ILogger.h"

namespace vacdm::log {
class LoggerAsyncBase : public ILogger {
   protected:
    struct LogMessage {
        LogLevel level;
        std::string message;
        std::source_location location;
        std::chrono::system_clock::time_point timestamp;
    };

    virtual void emitLog(const LogMessage& logMsg) = 0;

    std::queue<LogMessage> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_worker;
    std::atomic<bool> m_running;

    std::condition_variable m_flushCv;

    void processLogs() {
        while (m_running || !m_queue.empty()) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !m_queue.empty() || !m_running; });

            while (!m_queue.empty()) {
                auto logMsg = std::move(m_queue.front());
                m_queue.pop();
                lock.unlock();

                this->emitLog(logMsg);

                lock.lock();
            }

            m_flushCv.notify_all();
        }
    }

   public:
    LoggerAsyncBase() : m_running(true), m_worker(&LoggerAsyncBase::processLogs, this) {};
    virtual ~LoggerAsyncBase() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
            m_cv.notify_all();  // notify worker thread to exit
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_flushCv.wait(lock, [this]() { return m_queue.empty(); });
        }

        if (m_worker.joinable()) {
            m_worker.join();
        }
    };

    void LoggerAsyncBase::log(const LogLevel level, const std::string& message,
                              const std::source_location location = std::source_location::current()) {
        LogMessage logMsg{level, message, location, std::chrono::system_clock::now()};

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(logMsg));
        }
        m_cv.notify_one();
    }
};
}  // namespace vacdm::log