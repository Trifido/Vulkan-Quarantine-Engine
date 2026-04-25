#pragma once

#include "IEditorPanel.h"
#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Core/QEGizmoController.h>
#include <QuarantineEditor/Core/EditorSelectionManager.h>
#include <QuarantineEditor/Core/EditorPickingSystem.h>
#include <QuarantineEditor/Commands/EditorCommandManager.h> 
#include <QuarantineEditor/Rendering/EditorViewportResources.h>
#include <QuarantineEditor/Commands/TransformCommand.h>
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
