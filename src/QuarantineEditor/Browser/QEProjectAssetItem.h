#pragma once

#include <string>
#include <vector>
#include <memory>

enum class QEAssetType
{
    Folder,
    Scene,
    Material,
    Shader,
    Graph,
    Spv,
    Texture,
    Mesh,
    Animation,
    NavigateUp,
    Unknown
};

struct QEProjectAssetItem
{
    std::string Name;
    std::string AbsolutePath;
    std::string RelativePath;

    QEAssetType Type = QEAssetType::Unknown;
    bool IsDirectory = false;

    QEProjectAssetItem* Parent = nullptr;
    std::vector<std::shared_ptr<QEProjectAssetItem>> Children;
};
