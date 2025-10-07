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
    QECamera* _activeCamera = nullptr;
    QECamera* _editorCamera = nullptr;
    QECamera* _gameCamera = nullptr;

public:
    QESessionManager();

    const bool IsEditor() { return _isEditor; }
    void SetEditorMode(bool value);
    void AddSceneCamera(QECamera* camera) { _gameCamera = camera; }
    QECamera* ActiveCamera() { return _activeCamera; }
    QECamera* EditorCamera() { return _editorCamera; }

    void CleanCameras();
    void FreeCameraResources();
    void UpdateViewportSize();
};

#endif // !QE_SESSION_MANAGER


