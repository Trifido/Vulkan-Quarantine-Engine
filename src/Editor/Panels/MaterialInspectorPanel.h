#pragma once

#include "IEditorPanel.h"
#include <memory>
#include <functional>
#include <filesystem>

class GameObjectManager;
class MaterialManager;
struct EditorContext;
class EditorSelectionManager;
class QEMaterial;

class MaterialInspectorPanel : public IEditorPanel
{
public:
    MaterialInspectorPanel(
        GameObjectManager* gameObjectManager,
        MaterialManager* materialManager,
        EditorContext* editorContext,
        EditorSelectionManager* selectionManager,
        std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor);

    const char* GetName() const override { return "Material Inspector"; }
    void Draw() override;

private:
    void DrawToolbar(const std::shared_ptr<class QEGameObject>& gameObject);
    void DrawMaterialEntry(const std::shared_ptr<QEMaterial>& material, int materialIndex);
    bool DrawAddMaterialPopup(const std::shared_ptr<class QEGameObject>& gameObject, const char* popupId);
    void DeleteMaterialAt(const std::shared_ptr<class QEGameObject>& gameObject, int materialIndex);

private:
    GameObjectManager* gameObjectManager = nullptr;
    MaterialManager* materialManager = nullptr;
    EditorContext* editorContext = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor;
};
