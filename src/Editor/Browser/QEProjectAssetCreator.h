#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

class QEProjectAssetCreator
{
public:
    static bool CreateSceneAt(const fs::path& parentFolderPath, const std::string& baseName = "New Scene");
    static bool CreateMaterialAt(const fs::path& parentFolderPath, const std::string& baseName = "New Material");
    static bool CreateShaderAt(const fs::path& parentFolderPath, const std::string& baseName = "New Shader");

private:
    static bool ValidateParentFolder(const fs::path& parentFolderPath);
    static fs::path BuildUniqueFilePath(
        const fs::path& parentFolderPath,
        const std::string& baseName,
        const std::string& extension);
};
