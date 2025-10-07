#pragma once

#ifndef QE_SESSION_MANAGER
#define QE_SESSION_MANAGER

#include "QESingleton.h"
#include <CameraEditor.h>

class QESessionManager : public QESingleton<QESessionManager>
{
private:
    friend class QESingleton<QESessionManager>;
    bool _isEditor = true;
    bool _isDebugMode = false;
    QECamera* _activeCamera = nullptr;
    QECamera* _editorCamera = nullptr;
    QECamera* _gameCamera = nullptr;

public:
    QESessionManager();

    const bool IsEditor() { return _isEditor; }
    const bool IsDebugMode() { return _isDebugMode; }
    void SetEditorMode(bool value);
    void SetDebugMode(bool value);
    void RegisterSceneCamera(QECamera* camera);
    QECamera* ActiveCamera() { return _activeCamera; }
    QECamera* EditorCamera() { return _editorCamera; }

    void CleanCameras();
    void FreeCameraResources();
    void UpdateViewportSize();

    void SetupEditor();
    void CleanEditorResources();

    void UpdateCullingScene();
    void CleanCullingResources();
};

#endif // !QE_SESSION_MANAGER


