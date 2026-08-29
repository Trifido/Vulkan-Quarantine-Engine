#pragma once

#include "QEConsoleLogSink.h"
#include "QEFileLogSink.h"

#include <filesystem>
#include <memory>
#include <string>

class QELogSession final
{
public:
    explicit QELogSession(std::string applicationName);
    ~QELogSession();

    QELogSession(const QELogSession&) = delete;
    QELogSession& operator=(const QELogSession&) = delete;

    bool IsFileLoggingEnabled() const noexcept;
    const std::filesystem::path& GetLogFilePath() const noexcept;

private:
    static std::filesystem::path ResolveLogDirectory(const std::string& applicationName);
    static std::filesystem::path BuildLogFilePath(
        const std::filesystem::path& directory,
        const std::string& applicationName);
    static void PruneOldLogs(const std::filesystem::path& directory);
    void LogBuildMetadata(const std::string& applicationName) const;

    QEConsoleLogSink _consoleSink;
    std::unique_ptr<QEFileLogSink> _fileSink;
    std::filesystem::path _logFilePath;
};

namespace QE
{
    using ::QELogSession;
}
