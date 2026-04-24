#pragma once

#ifndef QE_ANIMATION_GRAPH_ASSET_HELPER_H
#define QE_ANIMATION_GRAPH_ASSET_HELPER_H

#include <filesystem>
#include <string>
#include <vector>

class QEAnimationComponent;
namespace YAML { class Node; }

namespace QEAnimationGraphAssetHelper
{
    YAML::Node SerializeAnimationComponentReference(const QEAnimationComponent& component);

    std::filesystem::path GetGraphAssetDirectory(const QEAnimationComponent& component);
    std::filesystem::path ResolveGraphAssetPath(const QEAnimationComponent& component);
    std::string GetGraphAssetPathOrDefault(const QEAnimationComponent& component);
    std::vector<std::string> ListGraphAssets(const QEAnimationComponent& component);
    bool SaveGraphAsset(QEAnimationComponent& component);
    bool CreateGraphAsset(QEAnimationComponent& component, const std::string& baseName, bool assignToComponent = true);
    bool RenameGraphAsset(QEAnimationComponent& component, const std::string& newName);
    bool DeleteGraphAsset(QEAnimationComponent& component);
    bool LoadGraphAsset(QEAnimationComponent& component, const std::string& graphAssetPath);

    bool LoadAnimationComponentFromReference(
        QEAnimationComponent& component,
        const YAML::Node& componentNode);
}

#endif // !QE_ANIMATION_GRAPH_ASSET_HELPER_H
