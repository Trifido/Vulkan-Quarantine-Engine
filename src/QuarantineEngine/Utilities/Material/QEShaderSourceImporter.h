#pragma once

#ifndef QE_SHADER_SOURCE_IMPORTER_H
#define QE_SHADER_SOURCE_IMPORTER_H

#include <filesystem>
#include <string>

#include <QEProjectManager.h>

namespace fs = std::filesystem;

class QEShaderSourceImporter
{
public:
    static bool IsSupportedShaderSourceFile(const fs::path& path);

    static fs::path ImportShaderSource(
        const fs::path& inputFile,
        const fs::path& targetFolder,
        const QEImportProgressCallback& onProgress = nullptr);

private:
    static fs::path FindGlslcExecutable();
    static std::string GetShaderStageSuffix(const fs::path& inputFile);
    static fs::path BuildOutputPath(const fs::path& inputFile, const fs::path& targetFolder);
    static std::wstring BuildCommandLine(
        const fs::path& executablePath,
        const fs::path& inputFile,
        const fs::path& outputFile);
    static std::string ExecuteProcessCaptureOutput(
        const fs::path& executablePath,
        const std::wstring& commandLine,
        int& exitCode);
};



namespace QE
{
    using ::QEShaderSourceImporter;
} // namespace QE
// QE namespace aliases
#endif
