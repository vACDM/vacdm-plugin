#pragma once

#include <mutex>

#include <logging/Performance.h>

namespace vacdm {
namespace com {
    class PerformanceLock {
    private:
        vacdm::logging::Performance m_acquireTime;
        vacdm::logging::Performance m_criticalSectionTime;

        std::mutex m_lock;
    public:
        PerformanceLock(const std::string& component) :
            m_acquireTime("LockAcquire" + component),
            m_criticalSectionTime("CriticalSectionAcquire" + component),
            m_lock() {}

        void lock() {
            this->m_acquireTime.start();
            this->m_lock.lock();
            this->m_acquireTime.stop();

            this->m_criticalSectionTime.start();
        }

        void unlock() {
            this->m_lock.unlock();
            this->m_criticalSectionTime.stop();
        }
    };
}
}
