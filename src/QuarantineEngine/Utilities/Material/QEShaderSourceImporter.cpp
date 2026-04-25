#include "QEShaderSourceImporter.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <Logging/QELogMacros.h>

namespace
{
    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return value;
    }

    std::wstring QuotePathWide(const fs::path& path)
    {
        return L"\"" + path.wstring() + L"\"";
    }
}

bool QEShaderSourceImporter::IsSupportedShaderSourceFile(const fs::path& path)
{
    const std::string ext = ToLower(path.extension().string());

    return ext == ".vert" ||
        ext == ".frag" ||
        ext == ".geom" ||
        ext == ".tesc" ||
        ext == ".tese" ||
        ext == ".comp" ||
        ext == ".task" ||
        ext == ".mesh";
}

fs::path QEShaderSourceImporter::ImportShaderSource(
    const fs::path& inputFile,
    const fs::path& targetFolder,
    const QEImportProgressCallback& onProgress)
{
    if (!fs::exists(inputFile))
        throw std::runtime_error("Shader source file does not exist.");

    if (!IsSupportedShaderSourceFile(inputFile))
        throw std::runtime_error("Unsupported shader source extension.");

    if (onProgress)
        onProgress(0.10f, "Preparing", "Resolving compiler");

    const fs::path glslcPath = FindGlslcExecutable();
    if (glslcPath.empty())
    {
        throw std::runtime_error(
            "Could not find glslc.exe. Configure VULKAN_SDK or install the Vulkan SDK.");
    }

    const fs::path resolvedTargetFolder = QEProjectManager::ResolveProjectPath(targetFolder);
    std::error_code ec;
    fs::create_directories(resolvedTargetFolder, ec);
    if (ec)
    {
        throw std::runtime_error("Could not create destination folder for compiled shader.");
    }

    const fs::path outputFile = BuildOutputPath(inputFile, resolvedTargetFolder);

    if (onProgress)
        onProgress(0.25f, "Compiling", outputFile.filename().string());

    const std::wstring commandLine = BuildCommandLine(glslcPath, inputFile, outputFile);

    int exitCode = 0;
    const std::string compilerOutput = ExecuteProcessCaptureOutput(glslcPath, commandLine, exitCode);

    if (exitCode != 0)
    {
        if (!compilerOutput.empty())
        {
            QE_LOG_ERROR_CAT_F(
                "QEShaderSourceImporter",
                "Shader compilation failed for {}:\n{}",
                inputFile.string(),
                compilerOutput);
        }
        else
        {
            QE_LOG_ERROR_CAT_F(
                "QEShaderSourceImporter",
                "Shader compilation failed for {} with exit code {}",
                inputFile.string(),
                exitCode);
        }

        throw std::runtime_error(
            compilerOutput.empty()
            ? "glslc failed to compile the shader."
            : compilerOutput);
    }

    if (onProgress)
        onProgress(1.0f, "Completed", outputFile.filename().string());

    return outputFile;
}

fs::path QEShaderSourceImporter::FindGlslcExecutable()
{
    if (const char* vulkanSdk = std::getenv("VULKAN_SDK"))
    {
        fs::path candidate = fs::path(vulkanSdk) / "Bin" / "glslc.exe";
        if (fs::exists(candidate))
            return candidate;
    }

    const fs::path sdkRoot = "C:\\VulkanSDK";
    if (!fs::exists(sdkRoot) || !fs::is_directory(sdkRoot))
        return {};

    std::vector<fs::path> versions;
    for (const auto& entry : fs::directory_iterator(sdkRoot))
    {
        if (entry.is_directory())
            versions.push_back(entry.path());
    }

    std::sort(versions.begin(), versions.end(), std::greater<fs::path>());

    for (const auto& versionPath : versions)
    {
        const fs::path candidate = versionPath / "Bin" / "glslc.exe";
        if (fs::exists(candidate))
            return candidate;
    }

    return {};
}

std::string QEShaderSourceImporter::GetShaderStageSuffix(const fs::path& inputFile)
{
    const std::string ext = ToLower(inputFile.extension().string());

    if (ext == ".vert")
        return "vert";
    if (ext == ".frag")
        return "frag";
    if (ext == ".geom")
        return "geom";
    if (ext == ".tesc")
        return "tesc";
    if (ext == ".tese")
        return "tese";
    if (ext == ".comp")
        return "comp";
    if (ext == ".task")
        return "task";
    if (ext == ".mesh")
        return "mesh";

    throw std::runtime_error("Unsupported shader stage suffix.");
}

fs::path QEShaderSourceImporter::BuildOutputPath(const fs::path& inputFile, const fs::path& targetFolder)
{
    const std::string suffix = GetShaderStageSuffix(inputFile);
    const std::string baseName = inputFile.stem().string() + "_" + suffix;

    fs::path outputPath = targetFolder / (baseName + ".spv");

    int index = 1;
    while (fs::exists(outputPath))
    {
        outputPath = targetFolder / (baseName + "_" + std::to_string(index) + ".spv");
        ++index;
    }

    return outputPath;
}

std::wstring QEShaderSourceImporter::BuildCommandLine(
    const fs::path& executablePath,
    const fs::path& inputFile,
    const fs::path& outputFile)
{
    std::wostringstream commandLine;
    commandLine << QuotePathWide(executablePath)
        << L" "
        << QuotePathWide(inputFile)
        << L" -o "
        << QuotePathWide(outputFile);
    return commandLine.str();
}

std::string QEShaderSourceImporter::ExecuteProcessCaptureOutput(
    const fs::path& executablePath,
    const std::wstring& commandLine,
    int& exitCode)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
    {
        exitCode = -1;
        return "Failed to create output pipe for glslc.";
    }

    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0))
    {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        exitCode = -1;
        return "Failed to configure output pipe for glslc.";
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(STARTUPINFOW);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = writePipe;

    PROCESS_INFORMATION processInfo{};
    std::vector<wchar_t> commandBuffer(commandLine.begin(), commandLine.end());
    commandBuffer.push_back(L'\0');

    const BOOL created = CreateProcessW(
        executablePath.wstring().c_str(),
        commandBuffer.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    CloseHandle(writePipe);

    if (!created)
    {
        const DWORD lastError = GetLastError();
        CloseHandle(readPipe);
        exitCode = static_cast<int>(lastError);
        return "Failed to launch glslc process.";
    }

    std::string output;
    char buffer[512];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        output.append(buffer, bytesRead);
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD processExitCode = 0;
    if (!GetExitCodeProcess(processInfo.hProcess, &processExitCode))
        processExitCode = static_cast<DWORD>(-1);

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    CloseHandle(readPipe);

    exitCode = static_cast<int>(processExitCode);
    return output;
}
