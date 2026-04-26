#include "QECameraContext.h"
#include <SwapChainModule.h>
#include <GameObjectManager.h>
#include <SynchronizationModule.h>
#include <QECamera.h>
#include <DebugSystem/QEDebugSystem.h>
#include <Helpers/QEMemoryTrack.h>

QECameraContext::QECameraContext()
{
    auto deviceModule = DeviceModule::getInstance();
    this->activeCameraUBO = std::make_shared<UniformBufferObject>();
    this->activeCameraUBO->CreateUniformBuffer(sizeof(UniformCamera), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

bool QECameraContext::IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const
{
    return camera && camera->Owner && camera->Owner->IsActiveInHierarchy();
}

void QECameraContext::RegisterSceneCameras()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    _gameCamera = nullptr;

    const std::string excludedGameObjectName =
        (_cameraOverride && _cameraOverride->Owner) ? _cameraOverride->Owner->Name : "";

    _gameCamera = gameObjectManager->FindFirstComponentInScene<QECamera>(excludedGameObjectName);
}

void QECameraContext::FreeCameraResources()
{
    this->_cameraOverride.reset();
}

void QECameraContext::SetCameraOverride(const std::shared_ptr<QECamera>& camera)
{
    _cameraOverride = camera;
}

void QECameraContext::ClearCameraOverride()
{
    _cameraOverride.reset();
}

void QECameraContext::UpdateActiveCameraGPUData(uint32_t currentFrame)
{
    UpdateCameraGPUData(ActiveCamera(), currentFrame);
}

void QECameraContext::UpdateCameraGPUData(const std::shared_ptr<QECamera>& camera, uint32_t currentFrame)
{
    if (camera == nullptr || !camera->CameraData || this->activeCameraUBO == nullptr)
        return;

    auto deviceModule = DeviceModule::getInstance();

    void* data = nullptr;
    vkMapMemory(
        deviceModule->device,
        this->activeCameraUBO->uniformBuffersMemory[currentFrame],
        0,
        sizeof(UniformCamera),
        0,
        &data);

    memcpy(
        data,
        static_cast<const void*>(camera->CameraData.get()),
        sizeof(UniformCamera));

    vkUnmapMemory(deviceModule->device, this->activeCameraUBO->uniformBuffersMemory[currentFrame]);
}

void QECameraContext::UpdateCameraOverrideViewportSize(uint32_t width, uint32_t height)
{
    if (this->_cameraOverride == nullptr)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    this->_cameraOverride->UpdateViewportSize(extent);
    this->_cameraOverride->UpdateCamera();
}

void QECameraContext::UpdateGameCameraViewportSize(uint32_t width, uint32_t height)
{
    if (this->_gameCamera == nullptr)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    this->_gameCamera->UpdateViewportSize(extent);
    this->_gameCamera->UpdateCamera();
}

void QECameraContext::UpdateActiveCameraViewportSize(uint32_t width, uint32_t height)
{
    if (!_activeCamera)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    _activeCamera->UpdateViewportSize(extent);
    _activeCamera->UpdateCamera();
}

void QECameraContext::ResolveActiveCamera()
{
    if (!IsCameraActiveInHierarchy(_gameCamera))
    {
        _gameCamera = nullptr;
    }

    if (!IsCameraActiveInHierarchy(_cameraOverride))
    {
        _cameraOverride = nullptr;
    }

    _activeCamera = _cameraOverride ? _cameraOverride : _gameCamera;
}

std::shared_ptr<QECamera> QECameraContext::ActiveCamera() const
{
    if (IsCameraActiveInHierarchy(_activeCamera))
    {
        return _activeCamera;
    }

    if (IsCameraActiveInHierarchy(_cameraOverride))
        return _cameraOverride;

    if (IsCameraActiveInHierarchy(_gameCamera))
        return _gameCamera;

    return nullptr;
}

void QECameraContext::ClearSceneCameras()
{
    _gameCamera.reset();
    _activeCamera = _cameraOverride;
}

void QECameraContext::ShutdownPersistentResources()
{
    if (!this->activeCameraUBO)
        return;

    auto deviceModule = DeviceModule::getInstance();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        QE_DESTROY_BUFFER(deviceModule->device, this->activeCameraUBO->uniformBuffers[i], "QECameraContext::ShutdownPersistentResources");
        QE_FREE_MEMORY(deviceModule->device, this->activeCameraUBO->uniformBuffersMemory[i], "QECameraContext::ShutdownPersistentResources");
    }

    this->activeCameraUBO.reset();
}
