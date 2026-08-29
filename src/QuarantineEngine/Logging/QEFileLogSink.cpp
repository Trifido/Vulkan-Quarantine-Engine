#include "QEFileLogSink.h"

#include <chrono>
#include <iomanip>
#include <stdexcept>

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

    std::tm ToLocalTime(std::time_t time)
    {
        std::tm result{};
#ifdef _WIN32
        localtime_s(&result, &time);
#else
        localtime_r(&time, &result);
#endif
        return result;
    }
}

QEFileLogSink::QEFileLogSink(const std::filesystem::path& filePath)
    : _filePath(filePath)
{
    if (_filePath.empty())
    {
        throw std::invalid_argument("A log file path is required");
    }

    std::error_code error;
    std::filesystem::create_directories(_filePath.parent_path(), error);
    if (error)
    {
        throw std::runtime_error("Could not create log directory: " + error.message());
    }

    _stream.open(_filePath, std::ios::out | std::ios::trunc);
    if (!_stream.is_open())
    {
        throw std::runtime_error("Could not open log file: " + _filePath.string());
    }
}

QEFileLogSink::~QEFileLogSink()
{
    std::scoped_lock lock(_mutex);
    _stream.flush();
    _stream.close();
}

void QEFileLogSink::Write(const QELogEntry& entry)
{
    const auto timestamp = entry.Timestamp == std::chrono::system_clock::time_point{}
        ? std::chrono::system_clock::now()
        : entry.Timestamp;
    const auto time = std::chrono::system_clock::to_time_t(timestamp);
    const auto localTime = ToLocalTime(time);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        timestamp.time_since_epoch()) % 1000;

    std::scoped_lock lock(_mutex);
    _stream << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << milliseconds.count()
            << "] [" << ToString(entry.Level) << "]";

    if (!entry.Category.empty())
    {
        _stream << " [" << entry.Category << ']';
    }

    _stream << " [Thread " << entry.ThreadId << "] " << entry.Message << '\n';
    _stream.flush();
}

const std::filesystem::path& QEFileLogSink::GetFilePath() const noexcept
{
    return _filePath;
}
