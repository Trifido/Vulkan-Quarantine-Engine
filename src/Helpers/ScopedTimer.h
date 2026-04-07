#include <string>
#include <chrono>
#include <Logging/QELogMacros.h>

static thread_local int g_profilerDepth = 0;

class ScopedTimer
{
public:
    ScopedTimer(const std::string& name)
        : m_name(name)
        , m_start(std::chrono::high_resolution_clock::now())
    {
        g_profilerDepth++;
    }

    ~ScopedTimer()
    {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
        const double durationMs = static_cast<double>(durationUs) / 1000.0;

        std::string indent;
        indent.reserve(static_cast<size_t>(g_profilerDepth) * 2);

        for (int i = 0; i < g_profilerDepth; ++i)
        {
            indent += "  ";
        }

        QE_LOG_INFO_CAT(
            "Profiler",
            indent + m_name + " -> " + std::to_string(durationMs) + " ms"
        );

        g_profilerDepth--;
    }

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#ifndef PROFILE_SCOPE
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name)
#endif
