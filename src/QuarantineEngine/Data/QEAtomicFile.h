#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace QE::Persistence
{
struct AtomicWriteOptions
{
    bool CreateBackup = true;
};

std::filesystem::path GetBackupPath(const std::filesystem::path& targetPath);

bool WriteTextFileAtomically(
    const std::filesystem::path& targetPath,
    std::string_view content,
    const AtomicWriteOptions& options = {},
    std::string* errorMessage = nullptr);
} // namespace QE::Persistence
