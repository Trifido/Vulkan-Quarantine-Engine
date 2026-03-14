#pragma once

#include "QEBaseApp.h"
#include <memory>
#include <vector>
#include <Editor/Core/EditorContext.h>
#include <Editor/Panels/IEditorPanel.h>

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
    std::vector<std::unique_ptr<IEditorPanel>> panels;
};
