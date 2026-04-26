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

    void RegisterSceneCameras();
    void SetFindNewSceneCamera(std::string cameraID);
    void FreeCameraResources();
    void SetCameraOverride(const std::shared_ptr<QECamera>& camera);
    void ClearCameraOverride();

    void UpdateActiveCameraGPUData(uint32_t currentFrame);
    void UpdateCameraGPUData(const std::shared_ptr<QECamera>& camera, uint32_t currentFrame);

    void UpdateGameCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateCameraOverrideViewportSize(uint32_t width, uint32_t height);
    void UpdateActiveCameraViewportSize(uint32_t width, uint32_t height);

    void UpdateCullingScene();
    void CleanCullingResources();

    void FindNewSceneCamera();
    void ResolveActiveCamera();

    void ResetSceneState();
    void ShutdownPersistentResources();

    std::shared_ptr<UniformBufferObject> GetCameraUBO() { return this->cameraUBO; }

    std::shared_ptr<QECamera> ActiveCamera() const;
    std::shared_ptr<QECamera> GameCamera() const { return _gameCamera; }
    std::shared_ptr<QECamera> CameraOverride() const { return _cameraOverride; }

    const QERenderTarget* GetRenderTargetOverride() const { return _renderTargetOverride; }
    void SetRenderTargetOverride(const QERenderTarget* renderTarget) { _renderTargetOverride = renderTarget; }

private:
    bool IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const;

    bool _newSceneCamera = false;

    std::string newCameraID;

    std::shared_ptr<QECamera> _activeCamera = nullptr;
    std::shared_ptr<QECamera> _gameCamera = nullptr;
    std::shared_ptr<QECamera> _cameraOverride = nullptr;
    const QERenderTarget* _renderTargetOverride = nullptr;
    std::shared_ptr<UniformBufferObject> cameraUBO = nullptr;
};


namespace QE
{
    using ::QECamera;
    using ::UniformBufferObject;
    using ::QESessionManager;
} // namespace QE
// QE namespace aliases
