#pragma once

#include "IEditorPanel.h"

class GameObjectManager;
struct EditorContext;
class EditorSelectionManager;

class InspectorPanel : public IEditorPanel
{
public:
    InspectorPanel(
        GameObjectManager* gameObjectManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager);

    const char* GetName() const override { return "Inspector"; }
    void Draw() override;

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
};
