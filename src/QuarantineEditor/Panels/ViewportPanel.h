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

class QECamera;

class ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel(
        EditorContext* editorContext,
        EditorViewportResources* viewportResources,
        EditorSelectionManager* selectionManager,
        QEGizmoController* gizmoController,
        EditorPickingSystem* pickingSystem,
        EditorCommandManager* commandManager,
        std::function<std::shared_ptr<QECamera>()> getEditorCamera,
        std::function<void(uint32_t, uint32_t)> resizeEditorCameraViewport);

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
    std::function<std::shared_ptr<QECamera>()> getEditorCamera;
    std::function<void(uint32_t, uint32_t)> resizeEditorCameraViewport;

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
