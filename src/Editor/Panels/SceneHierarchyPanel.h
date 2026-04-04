#pragma once

#include "IEditorPanel.h"

#include <Editor/Core/EditorSelectionManager.h>
#include <memory>

class GameObjectManager;
class QEGameObject;
struct EditorContext;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    SceneHierarchyPanel(GameObjectManager* gameObjectManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager);

    const char* GetName() const override { return "Scene Hierarchy"; }
    void Draw() override;

private:
    void DrawGameObjectNode(const std::shared_ptr<QEGameObject>& gameObject);
    void DrawWindowContextMenu();
    void HandleDeleteShortcut();

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;

    std::shared_ptr<QEGameObject> pendingDeleteGameObject = nullptr;
    bool pendingCreateEmptyGameObject = false;

//DRAG & DROP
private:
    bool IsDescendantOf(const std::shared_ptr<QEGameObject>& node, const std::shared_ptr<QEGameObject>& potentialAncestor) const;
    bool ReparentGameObject(const std::shared_ptr<QEGameObject>& child, const std::shared_ptr<QEGameObject>& newParent);

    std::shared_ptr<QEGameObject> draggingGameObject = nullptr;
    bool dragDropHandledThisFrame = false;
};
