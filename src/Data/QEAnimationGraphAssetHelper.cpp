#include "QEAnimationGraphAssetHelper.h"

#include <QEAnimationComponent.h>
#include <QEGeometryComponent.h>
#include <QEProjectManager.h>
#include <Logging/QELogMacros.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    constexpr const char* kGraphAssetExtension = ".qegraph";
    constexpr const char* kDefaultGraphBaseName = "default";

    std::string BuildIndexedName(const std::string& baseName, int index)
    {
        if (index <= 0)
        {
            return baseName;
        }

        return baseName + " " + std::to_string(index);
    }

    fs::path BuildDefaultGraphAssetPath(const QEAnimationComponent& component)
    {
        return QEAnimationGraphAssetHelper::GetGraphAssetDirectory(component) / (std::string(kDefaultGraphBaseName) + kGraphAssetExtension);
    }

    fs::path BuildUniqueGraphAssetPath(const fs::path& parentFolder, const std::string& baseName)
    {
        const std::string sanitizedBaseName = baseName.empty() ? std::string(kDefaultGraphBaseName) : baseName;
        int index = 0;
        while (true)
        {
            const fs::path candidatePath = parentFolder / (BuildIndexedName(sanitizedBaseName, index) + kGraphAssetExtension);
            if (!fs::exists(candidatePath))
            {
                return candidatePath;
            }

            ++index;
        }
    }

    fs::path ResolveGraphAssetPathImpl(const QEAnimationComponent& component)
    {
        if (!component.GetGraphAssetPath().empty())
        {
            return QEProjectManager::ResolveProjectPath(component.GetGraphAssetPath());
        }

        return BuildDefaultGraphAssetPath(component);
    }

    YAML::Node BuildGraphAssetNode(const QEAnimationComponent& component)
    {
        YAML::Node root;
        root["Version"] = 1;
        root["EntryStateId"] = component.GetCurrentState().Id;
        root["States"] = component.GetAnimationStates();
        root["Transitions"] = component.GetAnimationTransitions();
        root["BoolParameters"] = component.GetBoolParameterNames();
        root["IntParameters"] = component.GetIntParameterNames();
        root["FloatParameters"] = component.GetFloatParameterNames();
        root["TriggerParameters"] = component.GetTriggerParameterNames();
        return root;
    }

    YAML::Node BuildEmptyGraphAssetNode()
    {
        YAML::Node root;
        root["Version"] = 1;
        root["EntryStateId"] = "";
        root["States"] = YAML::Node(YAML::NodeType::Sequence);
        root["Transitions"] = YAML::Node(YAML::NodeType::Sequence);
        root["BoolParameters"] = YAML::Node(YAML::NodeType::Sequence);
        root["IntParameters"] = YAML::Node(YAML::NodeType::Sequence);
        root["FloatParameters"] = YAML::Node(YAML::NodeType::Sequence);
        root["TriggerParameters"] = YAML::Node(YAML::NodeType::Sequence);
        return root;
    }

    bool WriteGraphAssetNode(const fs::path& assetPath, const YAML::Node& assetNode)
    {
        if (assetPath.empty())
        {
            return false;
        }

        std::error_code ec;
        fs::create_directories(assetPath.parent_path(), ec);
        if (ec)
        {
            QE_LOG_ERROR_CAT_F("QEAnimationGraphAssetHelper", "Could not create folder for {}", assetPath.string());
            return false;
        }

        YAML::Emitter out;
        out << assetNode;

        std::ofstream file(assetPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            QE_LOG_ERROR_CAT_F("QEAnimationGraphAssetHelper", "Could not open graph asset {}", assetPath.string());
            return false;
        }

        file << out.c_str();
        return true;
    }

    bool WriteGraphAssetFile(const fs::path& assetPath, const QEAnimationComponent& component)
    {
        return WriteGraphAssetNode(assetPath, BuildGraphAssetNode(component));
    }

    bool ApplyGraphAssetNode(QEAnimationComponent& component, const YAML::Node& root)
    {
        const std::vector<AnimationState> states =
            root["States"] ? root["States"].as<std::vector<AnimationState>>() : std::vector<AnimationState>{};
        const std::vector<QETransition> transitions =
            root["Transitions"] ? root["Transitions"].as<std::vector<QETransition>>() : std::vector<QETransition>{};
        const std::string entryStateId =
            root["EntryStateId"] ? root["EntryStateId"].as<std::string>() : std::string{};

        component.SetBoolParameterNames(
            root["BoolParameters"] ? root["BoolParameters"].as<std::vector<std::string>>() : std::vector<std::string>{});
        component.SetIntParameterNames(
            root["IntParameters"] ? root["IntParameters"].as<std::vector<std::string>>() : std::vector<std::string>{});
        component.SetFloatParameterNames(
            root["FloatParameters"] ? root["FloatParameters"].as<std::vector<std::string>>() : std::vector<std::string>{});
        component.SetTriggerParameterNames(
            root["TriggerParameters"] ? root["TriggerParameters"].as<std::vector<std::string>>() : std::vector<std::string>{});
        component.SetStateMachineData(states, transitions, entryStateId);

        return true;
    }
}

namespace QEAnimationGraphAssetHelper
{
    fs::path GetGraphAssetDirectory(const QEAnimationComponent& component)
    {
        if (!component.Owner)
        {
            return {};
        }

        auto geometryComponent = component.Owner->GetComponent<QEGeometryComponent>();
        if (!geometryComponent)
        {
            return {};
        }

        fs::path meshPath;
        if (auto* mesh = geometryComponent->GetMesh())
        {
            meshPath = mesh->FilePath;
        }

        if (meshPath.empty())
        {
            meshPath = geometryComponent->Path;
        }

        if (meshPath.empty())
        {
            return {};
        }

        meshPath = QEProjectManager::ResolveProjectPath(meshPath);
        return meshPath.parent_path().parent_path() / ANIMATION_FOLDER;
    }

    fs::path ResolveGraphAssetPath(const QEAnimationComponent& component)
    {
        return ResolveGraphAssetPathImpl(component);
    }

    std::string GetGraphAssetPathOrDefault(const QEAnimationComponent& component)
    {
        if (!component.GetGraphAssetPath().empty())
        {
            return component.GetGraphAssetPath();
        }

        const fs::path defaultPath = BuildDefaultGraphAssetPath(component);
        if (defaultPath.empty())
        {
            return {};
        }

        return QEProjectManager::ToProjectRelativePath(defaultPath);
    }

    std::vector<std::string> ListGraphAssets(const QEAnimationComponent& component)
    {
        std::vector<std::string> assetPaths;
        const fs::path assetDirectory = GetGraphAssetDirectory(component);
        if (assetDirectory.empty() || !fs::exists(assetDirectory))
        {
            return assetPaths;
        }

        for (const auto& entry : fs::directory_iterator(assetDirectory))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (entry.path().extension() != kGraphAssetExtension)
            {
                continue;
            }

            assetPaths.push_back(QEProjectManager::ToProjectRelativePath(entry.path()));
        }

        std::sort(assetPaths.begin(), assetPaths.end());
        return assetPaths;
    }

    bool SaveGraphAsset(QEAnimationComponent& component)
    {
        const fs::path assetPath = ResolveGraphAssetPathImpl(component);
        if (!WriteGraphAssetFile(assetPath, component))
        {
            return false;
        }

        component.SetGraphAssetPath(QEProjectManager::ToProjectRelativePath(assetPath));
        return true;
    }

    bool CreateGraphAsset(QEAnimationComponent& component, const std::string& baseName, bool assignToComponent)
    {
        const fs::path assetDirectory = GetGraphAssetDirectory(component);
        if (assetDirectory.empty())
        {
            return false;
        }

        const fs::path assetPath = BuildUniqueGraphAssetPath(assetDirectory, baseName);
        if (!WriteGraphAssetNode(assetPath, BuildEmptyGraphAssetNode()))
        {
            return false;
        }

        if (assignToComponent)
        {
            component.SetGraphAssetPath(QEProjectManager::ToProjectRelativePath(assetPath));
        }

        return true;
    }

    bool RenameGraphAsset(QEAnimationComponent& component, const std::string& newName)
    {
        if (newName.empty())
        {
            return false;
        }

        const fs::path currentAssetPath = ResolveGraphAssetPathImpl(component);
        if (currentAssetPath.empty() || !fs::exists(currentAssetPath))
        {
            return false;
        }

        const fs::path renamedPath = currentAssetPath.parent_path() / (newName + kGraphAssetExtension);
        if (currentAssetPath == renamedPath)
        {
            component.SetGraphAssetPath(QEProjectManager::ToProjectRelativePath(renamedPath));
            return true;
        }

        if (fs::exists(renamedPath))
        {
            return false;
        }

        if (!QEProjectManager::RenamePath(currentAssetPath, renamedPath.filename().string()))
        {
            return false;
        }

        component.SetGraphAssetPath(QEProjectManager::ToProjectRelativePath(renamedPath));
        return true;
    }

    bool DeleteGraphAsset(QEAnimationComponent& component)
    {
        const fs::path currentAssetPath = ResolveGraphAssetPathImpl(component);
        if (currentAssetPath.empty() || !fs::exists(currentAssetPath))
        {
            return false;
        }

        const std::string currentRelativePath = QEProjectManager::ToProjectRelativePath(currentAssetPath);
        auto availableAssets = ListGraphAssets(component);
        availableAssets.erase(
            std::remove(availableAssets.begin(), availableAssets.end(), currentRelativePath),
            availableAssets.end());

        if (!QEProjectManager::DeletePath(currentAssetPath, false))
        {
            return false;
        }

        if (!availableAssets.empty())
        {
            return LoadGraphAsset(component, availableAssets.front());
        }

        if (!CreateGraphAsset(component, kDefaultGraphBaseName, true))
        {
            return false;
        }

        return LoadGraphAsset(component, component.GetGraphAssetPath());
    }

    bool LoadGraphAsset(QEAnimationComponent& component, const std::string& graphAssetPath)
    {
        if (graphAssetPath.empty())
        {
            return false;
        }

        component.SetGraphAssetPath(graphAssetPath);

        const fs::path assetPath = ResolveGraphAssetPathImpl(component);
        YAML::Node root;
        try
        {
            root = YAML::LoadFile(assetPath.string());
        }
        catch (const YAML::Exception& e)
        {
            QE_LOG_ERROR_CAT_F("QEAnimationGraphAssetHelper", "Could not load graph asset {} ({})", assetPath.string(), e.what());
            return false;
        }

        return ApplyGraphAssetNode(component, root);
    }

    YAML::Node SerializeAnimationComponentReference(const QEAnimationComponent& component)
    {
        YAML::Node node;
        node["type"] = component.getTypeName();
        node["id"] = component.id;
        node["AutoStart"] = component.GetAutoStart();

        const fs::path assetPath = ResolveGraphAssetPathImpl(component);
        if (WriteGraphAssetFile(assetPath, component))
        {
            node["GraphAssetPath"] = QEProjectManager::ToProjectRelativePath(assetPath);
        }
        else if (!component.GetGraphAssetPath().empty())
        {
            node["GraphAssetPath"] = component.GetGraphAssetPath();
        }

        return node;
    }

    bool LoadAnimationComponentFromReference(
        QEAnimationComponent& component,
        const YAML::Node& componentNode)
    {
        if (!componentNode || !componentNode["GraphAssetPath"])
        {
            return false;
        }

        component.SetGraphAssetPath(componentNode["GraphAssetPath"].as<std::string>());

        const fs::path assetPath = ResolveGraphAssetPathImpl(component);
        YAML::Node root;
        try
        {
            root = YAML::LoadFile(assetPath.string());
        }
        catch (const YAML::Exception& e)
        {
            QE_LOG_ERROR_CAT_F("QEAnimationGraphAssetHelper", "Could not load graph asset {} ({})", assetPath.string(), e.what());
            return false;
        }

        return ApplyGraphAssetNode(component, root);
    }
}
