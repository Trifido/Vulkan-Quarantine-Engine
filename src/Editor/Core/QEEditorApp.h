#pragma once
#include "QEBaseApp.h"

class QEEditorApp : public QEBaseApp
{
public:
    ~QEEditorApp() override = default;

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
};

