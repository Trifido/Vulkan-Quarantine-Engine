#pragma once

#include <memory>
#include <string>
#include <glm/glm.hpp>

class GameObjectManager;
class EditorContext;
class QECamera;

class EditorCameraService
{
public:
    void EnsureEditorCamera(GameObjectManager* gameObjectManager);
    std::shared_ptr<QECamera> GetEditorCamera() const { return _editorCamera; }
    void UpdateInputState(EditorContext* editorContext) const;
    glm::vec3 GetSpawnPositionInFront(float distance) const;

private:
    std::shared_ptr<QECamera> _editorCamera;
};
