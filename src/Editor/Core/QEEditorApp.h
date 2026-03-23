#pragma once

#include "QEBaseApp.h"
#include <memory>
#include <vector>

class IEditorPanel;
class EditorContext;
class EditorSelectionManager;
class QEGizmoController;
class EditorPickingSystem;
class EditorViewportResources;
class EditorCommandManager;
class QERenderTarget;
class QEProjectBrowserPanel;

class QEEditorApp : public QEBaseApp
{
public:
    QEEditorApp();
    ~QEEditorApp() override;

    QEEditorApp(const QEEditorApp&) = delete;
    QEEditorApp& operator=(const QEEditorApp&) = delete;
    QEEditorApp(QEEditorApp&&) = delete;
    QEEditorApp& operator=(QEEditorApp&&) = delete;

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
    std::unique_ptr<EditorPickingSystem> pickingSystem;
    std::unique_ptr<EditorCommandManager> commandManager;
    std::unique_ptr<QEProjectBrowserPanel> projectBrowserPanel;
    std::vector<std::unique_ptr<IEditorPanel>> panels;
};
