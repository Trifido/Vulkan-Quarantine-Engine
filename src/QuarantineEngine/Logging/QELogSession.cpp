#include "QELogSession.h"

#include "QEBuildInfo.generated.h"
#include "QELogMacros.h"
#include "QELogger.h"

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace
{
    constexpr std::size_t MaximumRetainedLogs = 20;

    std::string SanitizeName(std::string value)
    {
        for (char& character : value)
        {
            const bool isAllowed =
                (character >= 'a' && character <= 'z') ||
                (character >= 'A' && character <= 'Z') ||
                (character >= '0' && character <= '9') ||
                character == '-' || character == '_';
            if (!isAllowed)
            {
                character = '_';
            }
        }
        return value.empty() ? "QuarantineEngine" : value;
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

    unsigned long CurrentProcessId()
    {
#ifdef _WIN32
        return static_cast<unsigned long>(GetCurrentProcessId());
#else
        return 0;
#endif
    }

    const char* BuildConfiguration()
    {
#ifdef _DEBUG
        return "Debug";
#else
        return "Release";
#endif
    }
}

QELogSession::QELogSession(std::string applicationName)
{
    applicationName = SanitizeName(std::move(applicationName));
    QELogger::Get().AddSink(&_consoleSink);

    try
    {
        const std::filesystem::path directory = ResolveLogDirectory(applicationName);
        PruneOldLogs(directory);
        _logFilePath = BuildLogFilePath(directory, applicationName);
        _fileSink = std::make_unique<QEFileLogSink>(_logFilePath);
        QELogger::Get().AddSink(_fileSink.get());
    }
    catch (const std::exception& exception)
    {
        _fileSink.reset();
        _logFilePath.clear();
        QE_LOG_WARN_CAT_F("Logging", "Persistent logging is unavailable: {}", exception.what());
    }

    QE_LOG_INFO_CAT_F("Logging", "Started {} log session", applicationName);
    if (_fileSink)
    {
        QE_LOG_INFO_CAT_F("Logging", "Log file: {}", _logFilePath.string());
    }
    LogBuildMetadata(applicationName);
}

QELogSession::~QELogSession()
{
    QE_LOG_INFO_CAT("Logging", "Log session finished");

    if (_fileSink)
    {
        QELogger::Get().RemoveSink(_fileSink.get());
        _fileSink.reset();
    }
    QELogger::Get().RemoveSink(&_consoleSink);
}

bool QELogSession::IsFileLoggingEnabled() const noexcept
{
    return _fileSink != nullptr;
}

const std::filesystem::path& QELogSession::GetLogFilePath() const noexcept
{
    return _logFilePath;
}

std::filesystem::path QELogSession::ResolveLogDirectory(const std::string& applicationName)
{
    std::filesystem::path root;
#ifdef _WIN32
    if (const char* localAppData = std::getenv("LOCALAPPDATA"))
    {
        root = localAppData;
    }
#else
    if (const char* stateHome = std::getenv("XDG_STATE_HOME"))
    {
        root = stateHome;
    }
#endif
    if (root.empty())
    {
        root = std::filesystem::temp_directory_path();
    }

    return root / "QuarantineEngine" / "Logs" / SanitizeName(applicationName);
}

std::filesystem::path QELogSession::BuildLogFilePath(
    const std::filesystem::path& directory,
    const std::string& applicationName)
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto localTime = ToLocalTime(time);

    std::ostringstream filename;
    filename << SanitizeName(applicationName) << '_'
             << std::put_time(&localTime, "%Y%m%d_%H%M%S") << '_'
             << CurrentProcessId() << ".log";
    return directory / filename.str();
}

void QELogSession::PruneOldLogs(const std::filesystem::path& directory)
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
    {
        throw std::system_error(error, "Could not create persistent log directory");
    }

    std::vector<std::filesystem::directory_entry> logs;
    for (std::filesystem::directory_iterator iterator(directory, error), end;
         !error && iterator != end;
         iterator.increment(error))
    {
        if (iterator->is_regular_file(error) && iterator->path().extension() == ".log")
        {
            logs.push_back(*iterator);
        }
    }
    if (error)
    {
        throw std::system_error(error, "Could not enumerate persistent logs");
    }

    std::sort(logs.begin(), logs.end(), [](const auto& left, const auto& right)
        {
            return left.path().filename() > right.path().filename();
        });

    const std::size_t retainedBeforeNewLog = MaximumRetainedLogs - 1;
    for (std::size_t index = retainedBeforeNewLog; index < logs.size(); ++index)
    {
        std::filesystem::remove(logs[index].path(), error);
        error.clear();
    }
}

void QELogSession::LogBuildMetadata(const std::string& applicationName) const
{
    const std::string commit = std::string(QE_BUILD_GIT_COMMIT) +
        (QE_BUILD_GIT_DIRTY ? " (dirty)" : "");
    QE_LOG_INFO_CAT_F(
        "Build",
        "Application={} EngineVersion={} Configuration={} Commit={}",
        applicationName,
        QE_ENGINE_VERSION,
        BuildConfiguration(),
        commit);
    QE_LOG_INFO_CAT_F(
        "Build",
        "C++={} VulkanHeaders={}.{}.{}",
        static_cast<long long>(__cplusplus),
        VK_API_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE),
        VK_API_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE),
        VK_API_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE));

    std::error_code error;
    const auto workingDirectory = std::filesystem::current_path(error);
    if (!error)
    {
        QE_LOG_INFO_CAT_F("Build", "WorkingDirectory={}", workingDirectory.string());
    }
}
