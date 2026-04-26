#include "EditorSceneStateSerializer.h"

#include <QuarantineEditor/Core/EditorDebugSettings.h>
#include <QECamera.h>
#include <QEScene.h>
#include <QETransform.h>
#include <glm_yaml_conversions.h>
#include <yaml-cpp/yaml.h>
#include <Logging/QELogMacros.h>
#include <fstream>

namespace
{
    constexpr const char* kEditorSceneStateExtension = ".qeeditor";
}

bool EditorSceneStateSerializer::Load(
    const QEScene& scene,
    const std::shared_ptr<QECamera>& editorCamera,
    EditorDebugSettings& debugSettings) const
{
    if (!editorCamera || !editorCamera->Owner)
        return false;

    const auto statePath = GetSceneStatePath(scene);
    if (statePath.empty() || !std::filesystem::exists(statePath))
        return false;

    YAML::Node root;
    try
    {
        root = YAML::LoadFile(statePath.string());
    }
    catch (const std::exception& e)
    {
        QE_LOG_WARN_CAT_F("EditorSceneStateSerializer", "Failed to load editor scene state '{}': {}", statePath.string(), e.what());
        return false;
    }

    if (auto cameraNode = root["EditorCamera"])
    {
        if (auto transform = editorCamera->Owner->GetComponent<QETransform>())
        {
            if (auto positionNode = cameraNode["LocalPosition"])
            {
                transform->SetLocalPosition(positionNode.as<glm::vec3>());
            }

            if (auto rotationNode = cameraNode["LocalRotation"])
            {
                transform->SetLocalRotation(rotationNode.as<glm::quat>());
            }
        }

        if (auto fovNode = cameraNode["Fov"])
        {
            editorCamera->SetFOV(fovNode.as<float>());
        }

        if (auto nearNode = cameraNode["Near"])
        {
            editorCamera->SetNear(nearNode.as<float>());
        }

        if (auto farNode = cameraNode["Far"])
        {
            editorCamera->SetFar(farNode.as<float>());
        }
    }

    if (auto displayNode = root["Display"])
    {
        if (auto showGridNode = displayNode["ShowEditorGrid"])
        {
            debugSettings.SetShowEditorGrid(showGridNode.as<bool>());
        }

        if (auto showColliderNode = displayNode["ShowColliderDebug"])
        {
            debugSettings.SetShowColliderDebug(showColliderNode.as<bool>());
        }

        if (auto showCullingNode = displayNode["ShowCullingAABBDebug"])
        {
            debugSettings.SetShowCullingAABBDebug(showCullingNode.as<bool>());
        }
    }

    editorCamera->UpdateCamera();
    return true;
}

bool EditorSceneStateSerializer::Save(
    const QEScene& scene,
    const std::shared_ptr<QECamera>& editorCamera,
    const EditorDebugSettings& debugSettings) const
{
    if (!editorCamera || !editorCamera->Owner)
        return false;

    auto transform = editorCamera->Owner->GetComponent<QETransform>();
    if (!transform)
        return false;

    const auto statePath = GetSceneStatePath(scene);
    if (statePath.empty())
        return false;

    std::error_code ec;
    std::filesystem::create_directories(statePath.parent_path(), ec);
    if (ec)
    {
        QE_LOG_WARN_CAT_F(
            "EditorSceneStateSerializer",
            "Failed to create editor scene state folder '{}': {}",
            statePath.parent_path().string(),
            ec.message());
        return false;
    }

    YAML::Node root;
    root["EditorCamera"]["LocalPosition"] = transform->localPosition;
    root["EditorCamera"]["LocalRotation"] = transform->localRotation;
    root["EditorCamera"]["Fov"] = editorCamera->GetFOV();
    root["EditorCamera"]["Near"] = editorCamera->GetNear();
    root["EditorCamera"]["Far"] = editorCamera->GetFar();
    root["Display"]["ShowEditorGrid"] = debugSettings.ShowEditorGrid();
    root["Display"]["ShowColliderDebug"] = debugSettings.ShowColliderDebug();
    root["Display"]["ShowCullingAABBDebug"] = debugSettings.ShowCullingAABBDebug();

    std::ofstream out(statePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        QE_LOG_WARN_CAT_F("EditorSceneStateSerializer", "Failed to save editor scene state '{}'", statePath.string());
        return false;
    }

    out << root;
    return true;
}

std::filesystem::path EditorSceneStateSerializer::GetSceneStatePath(const QEScene& scene) const
{
    const auto sceneFilePath = scene.GetSceneFilePath();
    if (sceneFilePath.empty())
        return {};

    auto statePath = sceneFilePath;
    statePath.replace_extension(kEditorSceneStateExtension);
    return statePath;
}
