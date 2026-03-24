#pragma once

#include "IEditorPanel.h"
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/QEGizmoController.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <Editor/Core/EditorPickingSystem.h>
#include <Editor/Commands/EditorCommandManager.h> 
#include <Editor/Rendering/EditorViewportResources.h>
#include <Editor/Commands/TransformCommand.h>
#include <functional>

class ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel(
        EditorContext* editorContext,
        EditorViewportResources* viewportResources,
        EditorSelectionManager* selectionManager,
        QEGizmoController* gizmoController,
        EditorPickingSystem* pickingSystem,
        EditorCommandManager* commandManager);

    const char* GetName() const override { return "Viewport"; }
    void Draw() override;

public:
    std::function<void(const std::string&)> OnAssetDroppedInViewport;

private:
    EditorContext* editorContext = nullptr;
    EditorViewportResources* viewportResources = nullptr;
    EditorSelectionManager* selectionManager = nullptr;
    QEGizmoController* gizmoController = nullptr;
    EditorPickingSystem* pickingSystem = nullptr;
    EditorCommandManager* commandManager = nullptr;

    bool wasUsingGizmoLastFrame = false;
    std::string gizmoTrackedObjectId;
    TransformState gizmoBeginState{};

private:
    void HandleViewportShortcuts();
    void HandlePicking();
    void HandleGizmoCommandTracking();
    void HandleAssetDropTarget();
    void HandleDroppedAssetPath(const std::string& assetPath);
};
