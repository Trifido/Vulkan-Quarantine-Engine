#pragma once
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <cstdint>

enum class QELogLevel
{
    Info,
    Warning,
    Error,
    Debug
};

struct QELogEntry
{
    QELogLevel Level = QELogLevel::Info;
    std::string Message;
    std::string Category;
    uint64_t FrameIndex = 0;
    std::chrono::system_clock::time_point Timestamp{};
    std::thread::id ThreadId{};
};


namespace QE
{
    using ::QELogLevel;
    using ::QELogEntry;
} // namespace QE
// QE namespace aliases
