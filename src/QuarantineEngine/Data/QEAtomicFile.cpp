#include "QEAtomicFile.h"

#include <atomic>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace
{
std::atomic_uint64_t g_temporaryFileCounter{ 0 };

void SetError(std::string* destination, const std::string& message)
{
    if (destination != nullptr)
        *destination = message;
}

fs::path MakeTemporaryPath(const fs::path& targetPath)
{
    const uint64_t counter = g_temporaryFileCounter.fetch_add(1, std::memory_order_relaxed);
#ifdef _WIN32
    const uint64_t processId = static_cast<uint64_t>(GetCurrentProcessId());
#else
    const uint64_t processId = static_cast<uint64_t>(getpid());
#endif
    fs::path temporaryFileName = targetPath.filename();
    temporaryFileName += ".tmp." + std::to_string(processId) + "." + std::to_string(counter);
    return targetPath.parent_path() / temporaryFileName;
}

bool FlushFileToDisk(const fs::path& path, std::string* errorMessage)
{
#ifdef _WIN32
    HANDLE file = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        SetError(errorMessage, "Could not reopen temporary file for flushing (Win32 error " +
            std::to_string(GetLastError()) + ")");
        return false;
    }

    const bool flushed = FlushFileBuffers(file) != FALSE;
    const DWORD flushError = flushed ? ERROR_SUCCESS : GetLastError();
    CloseHandle(file);
    if (!flushed)
    {
        SetError(errorMessage, "Could not flush temporary file (Win32 error " +
            std::to_string(flushError) + ")");
    }
    return flushed;
#else
    (void)path;
    (void)errorMessage;
    return true;
#endif
}

bool ReplaceFileAtomically(
    const fs::path& sourcePath,
    const fs::path& targetPath,
    std::string* errorMessage)
{
#ifdef _WIN32
    if (MoveFileExW(
        sourcePath.c_str(),
        targetPath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
    {
        SetError(errorMessage, "Atomic replace failed (Win32 error " +
            std::to_string(GetLastError()) + ")");
        return false;
    }
    return true;
#else
    if (std::rename(sourcePath.c_str(), targetPath.c_str()) != 0)
    {
        SetError(errorMessage, "Atomic replace failed");
        return false;
    }
    return true;
#endif
}

void RemoveTemporaryFile(const fs::path& path)
{
    std::error_code ignored;
    fs::remove(path, ignored);
}
} // namespace

namespace QE::Persistence
{
fs::path GetBackupPath(const fs::path& targetPath)
{
    return fs::path(targetPath.string() + ".bak");
}

bool WriteTextFileAtomically(
    const fs::path& targetPath,
    std::string_view content,
    const AtomicWriteOptions& options,
    std::string* errorMessage)
{
    if (errorMessage != nullptr)
        errorMessage->clear();

    if (targetPath.empty())
    {
        SetError(errorMessage, "Target path is empty");
        return false;
    }

    std::error_code ec;
    const fs::path parentPath = targetPath.parent_path();
    if (!parentPath.empty())
    {
        fs::create_directories(parentPath, ec);
        if (ec)
        {
            SetError(errorMessage, "Could not create target directory: " + ec.message());
            return false;
        }
    }

    if (fs::is_directory(targetPath, ec))
    {
        SetError(errorMessage, "Target path is a directory");
        return false;
    }
    ec.clear();

    const fs::path temporaryPath = MakeTemporaryPath(targetPath);
    {
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open())
        {
            SetError(errorMessage, "Could not create temporary file");
            return false;
        }

        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        output.flush();
        if (!output.good())
        {
            output.close();
            RemoveTemporaryFile(temporaryPath);
            SetError(errorMessage, "Could not write complete temporary file");
            return false;
        }
    }

    if (!FlushFileToDisk(temporaryPath, errorMessage))
    {
        RemoveTemporaryFile(temporaryPath);
        return false;
    }

    if (options.CreateBackup && fs::is_regular_file(targetPath, ec))
    {
        const fs::path backupPath = GetBackupPath(targetPath);
        const fs::path backupTemporaryPath = MakeTemporaryPath(backupPath);
        fs::copy_file(targetPath, backupTemporaryPath, fs::copy_options::overwrite_existing, ec);
        if (ec)
        {
            RemoveTemporaryFile(temporaryPath);
            RemoveTemporaryFile(backupTemporaryPath);
            SetError(errorMessage, "Could not stage backup: " + ec.message());
            return false;
        }

        if (!FlushFileToDisk(backupTemporaryPath, errorMessage) ||
            !ReplaceFileAtomically(backupTemporaryPath, backupPath, errorMessage))
        {
            RemoveTemporaryFile(temporaryPath);
            RemoveTemporaryFile(backupTemporaryPath);
            return false;
        }
    }
    ec.clear();

    if (!ReplaceFileAtomically(temporaryPath, targetPath, errorMessage))
    {
        RemoveTemporaryFile(temporaryPath);
        return false;
    }

    return true;
}
} // namespace QE::Persistence
