#include "QESessionManager.h"
#include <SwapChainModule.h>
#include <GameObjectManager.h>
#include <CullingSceneManager.h>
#include <SynchronizationModule.h>
#include <QECamera.h>
#include <DebugSystem/QEDebugSystem.h>
#include <Helpers/QEMemoryTrack.h>

QESessionManager::QESessionManager()
{
    auto deviceModule = DeviceModule::getInstance();
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->cameraUBO->CreateUniformBuffer(sizeof(UniformCamera), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

bool QESessionManager::IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const
{
    return camera && camera->Owner && camera->Owner->IsActiveInHierarchy();
}

void QESessionManager::RegisterSceneCameras()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    _gameCamera = nullptr;

    const std::string excludedGameObjectName =
        (_cameraOverride && _cameraOverride->Owner) ? _cameraOverride->Owner->Name : "";

    _gameCamera = gameObjectManager->FindFirstComponentInScene<QECamera>(excludedGameObjectName);
}

void QESessionManager::SetFindNewSceneCamera(std::string cameraID)
{
    if (cameraID != this->newCameraID)
    {
        this->newCameraID = cameraID;
        this->_newSceneCamera = true;
    }
}

void QESessionManager::FreeCameraResources()
{
    this->_cameraOverride.reset();
}

void QESessionManager::SetCameraOverride(const std::shared_ptr<QECamera>& camera)
{
    _cameraOverride = camera;
}

void QESessionManager::ClearCameraOverride()
{
    _cameraOverride.reset();
}

void QESessionManager::UpdateActiveCameraGPUData(uint32_t currentFrame)
{
    UpdateCameraGPUData(ActiveCamera(), currentFrame);
}

void QESessionManager::UpdateCameraGPUData(const std::shared_ptr<QECamera>& camera, uint32_t currentFrame)
{
    if (camera == nullptr || !camera->CameraData || this->cameraUBO == nullptr)
        return;

    auto deviceModule = DeviceModule::getInstance();

    void* data = nullptr;
    vkMapMemory(
        deviceModule->device,
        this->cameraUBO->uniformBuffersMemory[currentFrame],
        0,
        sizeof(UniformCamera),
        0,
        &data);

    memcpy(
        data,
        static_cast<const void*>(camera->CameraData.get()),
        sizeof(UniformCamera));

    vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame]);
}

void QESessionManager::UpdateCameraOverrideViewportSize(uint32_t width, uint32_t height)
{
    if (this->_cameraOverride == nullptr)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    this->_cameraOverride->UpdateViewportSize(extent);
    this->_cameraOverride->UpdateCamera();
}

void QESessionManager::UpdateGameCameraViewportSize(uint32_t width, uint32_t height)
{
    if (this->_gameCamera == nullptr)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    this->_gameCamera->UpdateViewportSize(extent);
    this->_gameCamera->UpdateCamera();
}

void QESessionManager::UpdateActiveCameraViewportSize(uint32_t width, uint32_t height)
{
    if (!_activeCamera)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    _activeCamera->UpdateViewportSize(extent);
    _activeCamera->UpdateCamera();
}

void QESessionManager::UpdateCullingScene()
{
    auto cullingSceneManager = CullingSceneManager::getInstance();
    if (cullingSceneManager)
    {
        cullingSceneManager->UpdateCullingScene();
    }
}

void QESessionManager::CleanCullingResources()
{
    auto cullingSceneManager = CullingSceneManager::getInstance();
    if (cullingSceneManager)
    {
        cullingSceneManager->ResetSceneState();
    }
}

void QESessionManager::FindNewSceneCamera()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    auto foundCamera = gameObjectManager->FindGameComponentInScene(this->newCameraID);
    this->_gameCamera = std::dynamic_pointer_cast<QECamera>(foundCamera);
}

void QESessionManager::ResolveActiveCamera()
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

std::shared_ptr<QECamera> QESessionManager::ActiveCamera() const
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

void QESessionManager::ResetSceneState()
{
    _gameCamera.reset();
    _activeCamera = _cameraOverride;

    _newSceneCamera = false;
    newCameraID.clear();

    CleanCullingResources();
}

void QESessionManager::ShutdownPersistentResources()
{
    if (!this->cameraUBO)
        return;

    auto deviceModule = DeviceModule::getInstance();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        QE_DESTROY_BUFFER(deviceModule->device, this->cameraUBO->uniformBuffers[i], "QESessionManager::ShutdownPersistentResources");
        QE_FREE_MEMORY(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], "QESessionManager::ShutdownPersistentResources");
    }

    this->cameraUBO.reset();
}
