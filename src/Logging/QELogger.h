#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <algorithm>
#include <format>
#include <utility>
#include <type_traits>

#include "QELogLevel.h"
#include "IQELogSink.h"

class QELogger
{
public:
    static QELogger& Get()
    {
        static QELogger instance;
        return instance;
    }

    void AddSink(IQELogSink* sink);
    void RemoveSink(IQELogSink* sink);

    void Info(const std::string& msg, const std::string& category = "");
    void Warning(const std::string& msg, const std::string& category = "");
    void Error(const std::string& msg, const std::string& category = "");
    void Debug(const std::string& msg, const std::string& category = "");

    template<typename... Args>
    void InfoFormat(const std::string& category, std::string_view fmt, Args&&... args)
    {
        Info(FormatMessage(fmt, std::forward<Args>(args)...), category);
    }

    template<typename... Args>
    void WarningFormat(const std::string& category, std::string_view fmt, Args&&... args)
    {
        Warning(FormatMessage(fmt, std::forward<Args>(args)...), category);
    }

    template<typename... Args>
    void ErrorFormat(const std::string& category, std::string_view fmt, Args&&... args)
    {
        Error(FormatMessage(fmt, std::forward<Args>(args)...), category);
    }

    template<typename... Args>
    void DebugFormat(const std::string& category, std::string_view fmt, Args&&... args)
    {
        Debug(FormatMessage(fmt, std::forward<Args>(args)...), category);
    }

private:
    QELogger() = default;

    void Push(QELogLevel level, const std::string& msg, const std::string& category);

    template<typename... Args>
    static std::string FormatMessage(std::string_view fmt, Args&&... args)
    {
        return FormatMessageImpl(
            fmt,
            std::decay_t<Args>(std::forward<Args>(args))...
        );
    }

    template<typename... Args>
    static std::string FormatMessageImpl(std::string_view fmt, Args... args)
    {
        return std::vformat(fmt, std::make_format_args(args...));
    }

private:
    std::mutex _mutex;
    std::vector<IQELogSink*> _sinks;
};
