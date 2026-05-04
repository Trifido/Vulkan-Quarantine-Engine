#pragma once

#include <memory>
#include <functional>

class EditorContext;
class EditorCommandManager;
class QECamera;
class QEEditorCameraController;
class PhysicsModule;

class EditorHeaderBar
{
public:
    EditorHeaderBar(
        EditorContext* editorContext,
        EditorCommandManager* commandManager,
        std::function<std::shared_ptr<QECamera>()> getEditorCamera,
        std::function<bool()> getShowEditorGrid,
        std::function<void(bool)> setShowEditorGrid,
        std::function<bool()> getShowColliderDebug,
        std::function<void(bool)> setShowColliderDebug,
        std::function<bool()> getShowCullingAABBDebug,
        std::function<void(bool)> setShowCullingAABBDebug,
        PhysicsModule* physicsModule);

    void Draw();
    void SetOnSaveRequested(const std::function<void()>& callback);

private:
    void DrawFileMenu();
    void DrawWindowMenu();
    void DrawDebugMenu();
    void DrawCameraSetup();
    void DrawPhysicsSetup();

    void DrawEditorCameraButton();
    void DrawEditorCameraPopup();
    void DrawPhysicsSettingsButton();
    void DrawPhysicsSettingsPopup();

private:
    EditorContext* editorContext = nullptr;
    EditorCommandManager* commandManager = nullptr;
    std::function<std::shared_ptr<QECamera>()> getEditorCamera;
    std::function<bool()> getShowEditorGrid;
    std::function<void(bool)> setShowEditorGrid;
    std::function<bool()> getShowColliderDebug;
    std::function<void(bool)> setShowColliderDebug;
    std::function<bool()> getShowCullingAABBDebug;
    std::function<void(bool)> setShowCullingAABBDebug;
    PhysicsModule* physicsModule = nullptr;

    std::function<void()> onSaveRequested;
};
