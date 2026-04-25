#pragma once
#include <string>
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
};


namespace QE
{
    using ::QELogLevel;
    using ::QELogEntry;
} // namespace QE
// QE namespace aliases
