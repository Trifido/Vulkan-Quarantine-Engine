#include <string>
#include <chrono>
#include <Logging/QELogMacros.h>

class ScopedTimer
{
public:
    ScopedTimer(const std::string& name)
        : m_name(name)
        , m_start(std::chrono::high_resolution_clock::now())
    {
    }

    ~ScopedTimer()
    {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
        const double durationMs = static_cast<double>(durationUs) / 1000.0;

        QE_LOG_INFO_CAT("Profiler", m_name + " -> " + std::to_string(durationMs) + " ms");
    }

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#ifndef PROFILE_SCOPE
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name)
#endif
