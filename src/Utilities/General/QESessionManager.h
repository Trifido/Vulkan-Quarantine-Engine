#pragma once

#ifndef QE_SESSION_MANAGER
#define QE_SESSION_MANAGER

#include "QESingleton.h"
#include <QECamera.h>

class QESessionManager : public QESingleton<QESessionManager>
{
private:
    friend class QESingleton<QESessionManager>;
    bool _isEditor = false;
    bool _isDebugMode = false;
    std::shared_ptr<QECamera> _activeCamera = nullptr;
    std::shared_ptr<QECamera> _editorCamera = nullptr;
    std::shared_ptr<QECamera> _gameCamera = nullptr;

    std::shared_ptr<UniformBufferObject> cameraUBO;

private:
    bool _newSceneCamera = false;
    std::string newCameraID = "";

public:
    const std::string NameCameraEditor = "QECameraEditor";

public:
    QESessionManager();

    const bool IsEditor() { return _isEditor; }
    const bool IsDebugMode() { return _isDebugMode; }
    void SetEditorMode(bool value);
    void SetDebugMode(bool value);
    void RegisterActiveSceneCamera();
    std::shared_ptr<QECamera> ActiveCamera() { return _activeCamera; }
    std::shared_ptr<QECamera> EditorCamera() { return _editorCamera; }
    std::shared_ptr<UniformBufferObject> GetCameraUBO() { return this->cameraUBO; }
    void SetFindNewSceneCamera(std::string cameraID);

    void CleanCameras();
    void FreeCameraResources();
    void UpdateActiveCameraGPUData();
    void UpdateViewportSize();

    void SetupEditor();
    void CleanEditorResources();

    void UpdateCullingScene();
    void CleanCullingResources();

    void FindNewSceneCamera();
};

#endif // !QE_SESSION_MANAGER


