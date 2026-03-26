#include <string>
#include <chrono>
#include <iostream>

static thread_local int g_profilerDepth = 0;

class ScopedTimer
{
public:
    ScopedTimer(const std::string& name)
        : m_name(name),
        m_start(std::chrono::high_resolution_clock::now())
    {
        g_profilerDepth++;
    }

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();

        for (int i = 0; i < g_profilerDepth; ++i)
            std::cout << "  ";

        std::cout << m_name << " -> " << (duration / 1000.0) << " ms\n";

        g_profilerDepth--;
    }

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#ifndef PROFILE_SCOPE
#define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name)
#endif
