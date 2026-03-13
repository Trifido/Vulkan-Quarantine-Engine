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
    bool IsEditorMode() const override { return true; }

    void OnPostInitVulkan() override;
    void OnPreCleanup() override;
    void OnSwapchainRecreated() override;

private:
    void InitializeImGui();
    void BeginImGuiFrame();
    void DrawEditorUI();
    void EndImGuiFrame();
    void ShutdownImGui();
    void SaveScene();

private:
    VkDescriptorPool        imguiPool{};
};

