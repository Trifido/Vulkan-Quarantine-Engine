#pragma once

#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <QESingleton.h>
#include <QERenderTarget.h>

class QECamera;
class UniformBufferObject;

class QECameraContext : public QESingleton<QECameraContext>
{
private:
    friend class QESingleton<QECameraContext>;

public:
    QECameraContext();

    void RegisterSceneCameras();
    void FreeCameraResources();
    void SetCameraOverride(const std::shared_ptr<QECamera>& camera);
    void ClearCameraOverride();

    void UpdateActiveCameraGPUData(uint32_t currentFrame);
    void UpdateCameraGPUData(const std::shared_ptr<QECamera>& camera, uint32_t currentFrame);

    void UpdateGameCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateCameraOverrideViewportSize(uint32_t width, uint32_t height);
    void UpdateActiveCameraViewportSize(uint32_t width, uint32_t height);

    void ResolveActiveCamera();

    void ClearSceneCameras();
    void ShutdownPersistentResources();

    std::shared_ptr<UniformBufferObject> GetActiveCameraUBO() { return this->activeCameraUBO; }

    std::shared_ptr<QECamera> ActiveCamera() const;
    std::shared_ptr<QECamera> GameCamera() const { return _gameCamera; }
    std::shared_ptr<QECamera> CameraOverride() const { return _cameraOverride; }

    const QERenderTarget* GetRenderTargetOverride() const { return _renderTargetOverride; }
    void SetRenderTargetOverride(const QERenderTarget* renderTarget) { _renderTargetOverride = renderTarget; }

private:
    bool IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const;

    std::shared_ptr<QECamera> _activeCamera = nullptr;
    std::shared_ptr<QECamera> _gameCamera = nullptr;
    std::shared_ptr<QECamera> _cameraOverride = nullptr;
    const QERenderTarget* _renderTargetOverride = nullptr;
    // GPU-side snapshot for the camera selected as active this frame.
    // Individual QECamera instances own their CPU-side CameraData.
    std::shared_ptr<UniformBufferObject> activeCameraUBO = nullptr;
};


namespace QE
{
    using ::QECamera;
    using ::UniformBufferObject;
    using ::QECameraContext;
} // namespace QE
// QE namespace aliases
