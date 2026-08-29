#pragma once

#include "IQELogSink.h"

#include <filesystem>
#include <fstream>
#include <mutex>

class QEFileLogSink final : public IQELogSink
{
public:
    explicit QEFileLogSink(const std::filesystem::path& filePath);
    ~QEFileLogSink() override;

    void Write(const QELogEntry& entry) override;
    const std::filesystem::path& GetFilePath() const noexcept;

private:
    std::filesystem::path _filePath;
    std::ofstream _stream;
    std::mutex _mutex;
};

namespace QE
{
    using ::QEFileLogSink;
}
