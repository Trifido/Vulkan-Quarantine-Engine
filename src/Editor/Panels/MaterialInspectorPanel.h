#pragma once

#include "IEditorPanel.h"
#include <memory>
#include <functional>

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
        EditorSelectionManager* selectionManager,
        std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor);

    const char* GetName() const override { return "Material Inspector"; }
    void Draw() override;

private:
    void DrawMaterialEntry(const std::shared_ptr<QEMaterial>& material, int materialIndex);

private:
    GameObjectManager* gameObjectManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor;
};
