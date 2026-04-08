#pragma once

#include "IEditorPanel.h"

#include <Editor/Core/EditorSelectionManager.h>
#include <memory>
#include <string>

class GameObjectManager;
class QEGameObject;
struct EditorContext;

class EditorSceneObjectFactory;
enum class QEPrimitiveType;

class SceneHierarchyPanel : public IEditorPanel
{
public:
    SceneHierarchyPanel(GameObjectManager* gameObjectManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager,
        EditorSceneObjectFactory* sceneObjectFactory);

    const char* GetName() const override { return "Scene Hierarchy"; }
    void Draw() override;

private:
    void DrawGameObjectNode(const std::shared_ptr<QEGameObject>& gameObject);
    void DrawAtmosphereNode();
    void DrawWindowContextMenu();
    void HandleDeleteShortcut();

    void DrawToolbar();
    void DrawCreateMenu();
    void CreatePrimitive(QEPrimitiveType type);

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    EditorSceneObjectFactory* sceneObjectFactory = nullptr;

    std::shared_ptr<QEGameObject> pendingDeleteGameObject = nullptr;
    bool pendingCreateEmptyGameObject = false;

//DRAG & DROP
private:
    bool IsDescendantOf(const std::shared_ptr<QEGameObject>& node, const std::shared_ptr<QEGameObject>& potentialAncestor) const;
    bool ReparentGameObject(const std::shared_ptr<QEGameObject>& child, const std::shared_ptr<QEGameObject>& newParent);

    std::shared_ptr<QEGameObject> draggingGameObject = nullptr;
    bool dragDropHandledThisFrame = false;

//RENAME QEGAMEOBJECTS
private:
    void StartRename(const std::shared_ptr<QEGameObject>& gameObject);
    void DrawRenamePopup();

    std::shared_ptr<QEGameObject> renamingGameObject = nullptr;
    std::string renameBuffer;
    bool openRenamePopup = false;
};
