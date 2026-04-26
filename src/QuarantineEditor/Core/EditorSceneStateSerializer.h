#pragma once

#include <filesystem>
#include <memory>

class QEScene;
class QECamera;
class EditorDebugSettings;

class EditorSceneStateSerializer
{
public:
    bool Load(const QEScene& scene, const std::shared_ptr<QECamera>& editorCamera, EditorDebugSettings& debugSettings) const;
    bool Save(const QEScene& scene, const std::shared_ptr<QECamera>& editorCamera, const EditorDebugSettings& debugSettings) const;

private:
    std::filesystem::path GetSceneStatePath(const QEScene& scene) const;
};
