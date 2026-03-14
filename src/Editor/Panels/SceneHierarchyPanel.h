#pragma once

#include "IEditorPanel.h"
#include <memory>

class GameObjectManager;
class QEGameObject;
struct EditorContext;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    SceneHierarchyPanel(GameObjectManager* gameObjectManager, EditorContext* editorContext);

    const char* GetName() const override { return "Scene Hierarchy"; }
    void Draw() override;

private:
    void DrawGameObjectNode(const std::shared_ptr<QEGameObject>& gameObject);

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
};
