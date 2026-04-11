#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <QESingleton.h>
#include <QERenderTarget.h>

class QECamera;
class UniformBufferObject;

class QESessionManager : public QESingleton<QESessionManager>
{
private:
    friend class QESingleton<QESessionManager>;

public:
    QESessionManager();

    void SetEditorMode(bool value);

    void SetDebugMode(bool value);
    void SetShowColliderDebug(bool value);
    void SetShowCullingAABBDebug(bool value);

    bool ShowColliderDebug() const { return _showColliderDebug; }
    bool ShowCullingAABBDebug() const { return _showCullingAABBDebug; }

    void RegisterActiveSceneCamera();
    void RegisterSceneCameras();
    void SetFindNewSceneCamera(std::string cameraID);
    void FreeCameraResources();

    void UpdateActiveCameraGPUData(uint32_t currentFrame);

    void UpdateEditorCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateGameCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateActiveCameraViewportSize(uint32_t width, uint32_t height);

    void SetupEditor();
    void CleanEditorResources();

    void UpdateCullingScene();
    void CleanCullingResources();

    void FindNewSceneCamera();
    void ResolveActiveCamera();

    void ResetSceneState();
    void ShutdownPersistentResources();

    std::shared_ptr<UniformBufferObject> GetCameraUBO() { return this->cameraUBO; }

    std::shared_ptr<QECamera> ActiveCamera() const { return _activeCamera; }
    std::shared_ptr<QECamera> EditorCamera() const { return _editorCamera; }
    std::shared_ptr<QECamera> GameCamera() const { return _gameCamera; }

    bool IsEditor() const { return _isEditor; }

public:
    std::string NameCameraEditor = "QECameraEditor";
    const QERenderTarget* ExtraRenderTarget = nullptr;

private:
    bool _isEditor = false;
    bool _isDebugMode = false;
    bool _newSceneCamera = false;

    bool _showColliderDebug = false;
    bool _showCullingAABBDebug = false;

    std::string newCameraID;

    std::shared_ptr<QECamera> _activeCamera = nullptr;
    std::shared_ptr<QECamera> _editorCamera = nullptr;
    std::shared_ptr<QECamera> _gameCamera = nullptr;

public:
    std::shared_ptr<UniformBufferObject> cameraUBO = nullptr;
};
