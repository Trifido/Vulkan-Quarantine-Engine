#include "QEConsoleLogSink.h"

#include <iostream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace
{
    const char* ToString(QELogLevel level)
    {
        switch (level)
        {
        case QELogLevel::Info:    return "Info";
        case QELogLevel::Warning: return "Warning";
        case QELogLevel::Error:   return "Error";
        case QELogLevel::Debug:   return "Debug";
        default:                  return "Unknown";
        }
    }

    std::string FormatLog(const QELogEntry& entry)
    {
        if (!entry.Category.empty())
        {
            return "[" + std::string(ToString(entry.Level)) + "] [" + entry.Category + "] " + entry.Message;
        }

        return "[" + std::string(ToString(entry.Level)) + "] " + entry.Message;
    }
}

void QEConsoleLogSink::Write(const QELogEntry& entry)
{
    const std::string formatted = FormatLog(entry);

    if (entry.Level == QELogLevel::Error)
    {
        std::cerr << formatted << std::endl;
    }
    else
    {
        std::cout << formatted << std::endl;
    }

#ifdef _WIN32
    OutputDebugStringA((formatted + "\n").c_str());
#endif
}
