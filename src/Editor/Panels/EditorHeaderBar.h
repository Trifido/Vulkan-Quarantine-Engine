#pragma once

#include <memory>
#include <functional>

class EditorContext;
class EditorCommandManager;
class QECamera;
class QECameraController;
class QESessionManager;

class EditorHeaderBar
{
public:
    EditorHeaderBar(
        EditorContext* editorContext,
        EditorCommandManager* commandManager,
        QESessionManager* sessionManager);

    void Draw();
    void SetOnSaveRequested(const std::function<void()>& callback);

private:
    void DrawFileMenu();
    void DrawWindowMenu();
    void DrawRightTools();

    void DrawEditorCameraButton();
    void DrawEditorCameraPopup();

private:
    EditorContext* editorContext = nullptr;
    EditorCommandManager* commandManager = nullptr;
    QESessionManager* sessionManager = nullptr;

    std::function<void()> onSaveRequested;
};
