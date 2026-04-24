#include "QEAnimationGraphAssetHelper.h"

#include <QEAnimationComponent.h>
#include <QEGeometryComponent.h>
#include <QEProjectManager.h>
#include <Logging/QELogMacros.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

namespace fs = std::filesystem;

namespace
{
    fs::path BuildDefaultGraphAssetPath(const QEAnimationComponent& component)
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
        return meshPath.parent_path().parent_path() / ANIMATION_FOLDER / (component.id + ".qegraph");
    }

    fs::path ResolveGraphAssetPath(const QEAnimationComponent& component)
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

    bool WriteGraphAssetFile(const fs::path& assetPath, const QEAnimationComponent& component)
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
        out << BuildGraphAssetNode(component);

        std::ofstream file(assetPath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            QE_LOG_ERROR_CAT_F("QEAnimationGraphAssetHelper", "Could not open graph asset {}", assetPath.string());
            return false;
        }

        file << out.c_str();
        return true;
    }
}

namespace QEAnimationGraphAssetHelper
{
    YAML::Node SerializeAnimationComponentReference(const QEAnimationComponent& component)
    {
        YAML::Node node;
        node["type"] = component.getTypeName();
        node["id"] = component.id;
        node["AutoStart"] = component.GetAutoStart();

        const fs::path assetPath = ResolveGraphAssetPath(component);
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

        const fs::path assetPath = ResolveGraphAssetPath(component);
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
