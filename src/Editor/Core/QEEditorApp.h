#pragma once

#include "QEBaseApp.h"
#include <memory>
#include <vector>
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <Editor/Core/QEGizmoController.h>
#include <Editor/Panels/IEditorPanel.h>
#include <QERenderTarget.h>
#include <Editor/Rendering/EditorViewportResources.h>

class QEEditorApp : public QEBaseApp
{
public:
    ~QEEditorApp() override;

protected:
    void OnInitialize() override;
    void OnShutdown() override;
    void OnBeginFrame() override;
    void OnEndFrame() override;

    void OnPostInitVulkan() override;
    void OnPreCleanup() override;
    void OnSwapchainRecreated() override;

    bool IsEditorMode() const override { return true; }
    const QERenderTarget* GetAdditionalSceneRenderTarget() const override;

private:
    void InitializeImGui();
    void BeginImGuiFrame();
    void DrawEditorUI();
    void EndImGuiFrame();
    void ShutdownImGui();

    void CreatePanels();
    void DrawDockspace();
    void HandleShortcuts();
    void SaveScene();

private:
    VkDescriptorPool imguiPool{};
    std::unique_ptr<EditorContext> editorContext;
    std::unique_ptr<EditorViewportResources> viewportResources;
    std::unique_ptr<EditorSelectionManager> selectionManager;
    std::unique_ptr<QEGizmoController> gizmoController;
    std::unique_ptr < std::vector<std::unique_ptr<IEditorPanel>>> panels;
};
