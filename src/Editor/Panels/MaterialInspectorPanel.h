#pragma once

#include "IEditorPanel.h"
#include <memory>

class GameObjectManager;
struct EditorContext;
class EditorSelectionManager;
class QEMaterial;

class MaterialInspectorPanel : public IEditorPanel
{
public:
    MaterialInspectorPanel(
        GameObjectManager* gameObjectManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager);

    const char* GetName() const override { return "Material Inspector"; }
    void Draw() override;

private:
    void DrawMaterial(const std::shared_ptr<QEMaterial>& material, int materialIndex);

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
};
