#include "QELogger.h"

void QELogger::AddSink(IQELogSink* sink)
{
    if (!sink)
        return;

    std::scoped_lock lock(_mutex);

    auto it = std::find(_sinks.begin(), _sinks.end(), sink);
    if (it == _sinks.end())
    {
        _sinks.push_back(sink);
    }
}

void QELogger::RemoveSink(IQELogSink* sink)
{
    std::scoped_lock lock(_mutex);
    _sinks.erase(std::remove(_sinks.begin(), _sinks.end(), sink), _sinks.end());
}

void QELogger::Info(const std::string& msg, const std::string& category)
{
    Push(QELogLevel::Info, msg, category);
}

void QELogger::Warning(const std::string& msg, const std::string& category)
{
    Push(QELogLevel::Warning, msg, category);
}

void QELogger::Error(const std::string& msg, const std::string& category)
{
    Push(QELogLevel::Error, msg, category);
}

void QELogger::Debug(const std::string& msg, const std::string& category)
{
    Push(QELogLevel::Debug, msg, category);
}

void QELogger::Push(QELogLevel level, const std::string& msg, const std::string& category)
{
    QELogEntry entry;
    entry.Level = level;
    entry.Message = msg;
    entry.Category = category;
    entry.FrameIndex = 0;

    std::vector<IQELogSink*> sinksCopy;
    {
        std::scoped_lock lock(_mutex);
        sinksCopy = _sinks;
    }

    for (IQELogSink* sink : sinksCopy)
    {
        if (sink)
        {
            sink->Write(entry);
        }
    }
}
