#pragma once

#include "IEditorPanel.h"
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/QEGizmoController.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <Editor/Core/EditorPickingSystem.h>
#include <Editor/Rendering/EditorViewportResources.h>

class ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel(
        EditorContext* editorContext,
        EditorViewportResources* viewportResources,
        EditorSelectionManager* selectionManager,
        QEGizmoController* gizmoController,
        EditorPickingSystem* pickingSystem);

    const char* GetName() const override { return "Viewport"; }
    void Draw() override;

    void HandleViewportShortcuts();

private:
    EditorContext* editorContext = nullptr;
    EditorViewportResources* viewportResources = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    QEGizmoController* gizmoController = nullptr;
    EditorPickingSystem* pickingSystem = nullptr;

    void HandlePicking();
};
