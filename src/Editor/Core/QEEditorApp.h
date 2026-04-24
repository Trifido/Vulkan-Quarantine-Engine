#pragma once

#include "QEBaseApp.h"
#include <memory>
#include <vector>
#include <filesystem>
#include <optional>

class IEditorPanel;
class EditorContext;
class EditorSelectionManager;
class QEGizmoController;
class EditorPickingSystem;
class EditorViewportResources;
class EditorCommandManager;
class QERenderTarget;
class QEProjectBrowserPanel;
class ViewportPanel;
class EditorHeaderBar;
class QEEditorConsole;
class QEEditorConsoleSink;
class QEConsoleLogSink;
class ConsolePanel;
class EditorSceneObjectFactory;
class QETextureViewerPanel;
class MaterialEditorPanel;
class ShaderEditorPanel;
class QEMaterial;

class QEEditorApp : public QEBaseApp
{
public:
    QEEditorApp();
    ~QEEditorApp() override;

    QEEditorApp(const QEEditorApp&) = delete;
    QEEditorApp& operator=(const QEEditorApp&) = delete;
    QEEditorApp(QEEditorApp&&) = delete;
    QEEditorApp& operator=(QEEditorApp&&) = delete;

    void QueueExternalDroppedFile(const std::filesystem::path& path);
    const std::vector<std::filesystem::path>& GetExternalDroppedFiles() const;
    void ClearExternalDroppedFiles();

protected:
    void OnInitialize() override;
    void OnShutdown() override;
    void OnFrameStart() override;
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
    void SetAdditionalSceneRenderTarget();
    void ShutdownImGui();

    void CreatePanels();
    void DrawDockspace();
    void HandleShortcuts();
    void SaveScene();
    void OpenScene(const std::filesystem::path& scenePath);
    void OpenTextureViewer(const std::filesystem::path& texturePath);
    void DrawTextureViewerPanels();
    void OpenMaterialEditor(const std::filesystem::path& materialPath);
    void OpenMaterialEditor(const std::shared_ptr<QEMaterial>& material);
    void OpenShaderEditor(const std::filesystem::path& shaderPath);

    void SpawnDroppedMesh(const std::string& assetPath);
    glm::vec3 GetSpawnPositionInFrontOfEditorCamera(float distance) const;
    void UpdateEditorCameraInputState();

    void FlushClosedTextureViewerPanels();

private:
    VkDescriptorPool imguiPool{};
    std::unique_ptr<EditorContext> editorContext;
    std::unique_ptr<EditorViewportResources> viewportResources;
    std::unique_ptr<EditorSelectionManager> selectionManager;
    std::unique_ptr<QEGizmoController> gizmoController;
    std::unique_ptr<EditorPickingSystem> pickingSystem;
    std::unique_ptr<EditorCommandManager> commandManager;
    std::unique_ptr<QEProjectBrowserPanel> projectBrowserPanel;
    QEProjectBrowserPanel* projectBrowserPanelPtr = nullptr;
    std::unique_ptr<ViewportPanel> viewportPanel = nullptr;
    std::unique_ptr<EditorHeaderBar> headerBar;
    std::vector<std::unique_ptr<IEditorPanel>> panels;
    std::vector<std::filesystem::path> _externalDroppedFiles;
    std::unique_ptr<QEEditorConsole> editorConsole;
    std::unique_ptr<QEEditorConsoleSink> editorConsoleSink;
    std::unique_ptr<QEConsoleLogSink> consoleLogSink;
    std::unique_ptr<EditorSceneObjectFactory> sceneObjectFactory;
    std::optional<std::filesystem::path> _pendingSceneOpenPath;

    std::vector<std::unique_ptr<QETextureViewerPanel>> _textureViewerPanels;
    std::vector<std::unique_ptr<QETextureViewerPanel>> _textureViewerPanelsPendingDestroy;
    MaterialEditorPanel* materialEditorPanelPtr = nullptr;
    ShaderEditorPanel* shaderEditorPanelPtr = nullptr;
};
