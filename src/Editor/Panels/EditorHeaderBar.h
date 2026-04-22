#pragma once

#include <memory>
#include <functional>

class EditorContext;
class EditorCommandManager;
class QECamera;
class QECameraController;
class QESessionManager;
class PhysicsModule;

class EditorHeaderBar
{
public:
    EditorHeaderBar(
        EditorContext* editorContext,
        EditorCommandManager* commandManager,
        QESessionManager* sessionManager,
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
    QESessionManager* sessionManager = nullptr;
    PhysicsModule* physicsModule = nullptr;

    std::function<void()> onSaveRequested;
};
