#pragma once

#include "IEditorPanel.h"
#include <memory>

class GameObjectManager;
class QEGameObject;
struct EditorContext;
class EditorSelectionManager;
class EditorCommandManager;

class InspectorPanel : public IEditorPanel
{
public:
    InspectorPanel(
        GameObjectManager* gameObjectManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager,
        EditorCommandManager* commandManager);

    const char* GetName() const override { return "Inspector"; }
    void Draw() override;

private:
    void DeleteSelectedComponent(const std::shared_ptr<QEGameObject>& gameObject);

    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    EditorCommandManager* commandManager = nullptr;
    int selectedComponentIndex = -1;
};
