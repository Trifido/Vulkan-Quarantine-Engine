#include "QESessionManager.h"
#include <SwapChainModule.h>
#include <GameObjectManager.h>
#include <CullingSceneManager.h>
#include <SynchronizationModule.h>
#include <QECameraController.h>
#include <DebugSystem/QEDebugSystem.h>
#include <Helpers/QEMemoryTrack.h>

QESessionManager::QESessionManager()
{
    auto deviceModule = DeviceModule::getInstance();
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->cameraUBO->CreateUniformBuffer(sizeof(UniformCamera), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void QESessionManager::SetEditorMode(bool value)
{
    this->_isEditor = value;

    auto gameObjectManager = GameObjectManager::getInstance();

    if (_editorCamera == nullptr)
    {
        std::shared_ptr<QEGameObject> cameraObject = gameObjectManager->GetGameObject(NameCameraEditor);

        if (cameraObject == nullptr)
        {
            cameraObject = std::make_shared<QEGameObject>(NameCameraEditor);
            cameraObject->AddComponent(std::make_shared<QECamera>(1280.0f, 720.0f));
            cameraObject->AddComponent(std::make_shared<QECameraController>());

            auto cameraTransform = cameraObject->GetComponent<QETransform>();
            cameraTransform->SetLocalEulerDegrees(glm::vec3(-45.0f, 0.0f, 0.0f));
            cameraTransform->SetLocalPosition(glm::vec3(0.0f, 10.0f, 10.0f));

            gameObjectManager->AddGameObject(cameraObject);

            _editorCamera = cameraObject->GetComponent<QECamera>();
        }
    }

    RegisterSceneCameras();
    ResolveActiveCamera();
}

void QESessionManager::SetShowColliderDebug(bool value)
{
    _showColliderDebug = value;

    auto debugSystem = QEDebugSystem::getInstance();
    if (debugSystem)
    {
        debugSystem->SetEnabled(value);
    }
}

void QESessionManager::SetShowCullingAABBDebug(bool value)
{
    _showCullingAABBDebug = value;

    auto cullingSceneManager = CullingSceneManager::getInstance();
    if (cullingSceneManager)
    {
        cullingSceneManager->DebugMode = value;
    }
}

void QESessionManager::SetShowEditorGrid(bool value)
{
    _showEditorGrid = value;
    if (_setEditorGridVisibilityCallback)
    {
        _setEditorGridVisibilityCallback(value);
    }
}

void QESessionManager::RegisterActiveSceneCamera()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    _gameCamera = nullptr;

    auto foundCamera = gameObjectManager->FindFirstComponentInScene<QECamera>(NameCameraEditor);
    if (foundCamera)
    {
        _gameCamera = foundCamera;
    }

    _activeCamera = (_gameCamera != nullptr) ? _gameCamera : _editorCamera;
}

bool QESessionManager::IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const
{
    return camera && camera->Owner && camera->Owner->IsActiveInHierarchy();
}

void QESessionManager::RegisterSceneCameras()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    _editorCamera = nullptr;
    _gameCamera = nullptr;

    auto editorCameraObject = gameObjectManager->GetGameObject(NameCameraEditor);
    if (editorCameraObject)
    {
        _editorCamera = editorCameraObject->GetComponent<QECamera>();
    }

    _gameCamera = gameObjectManager->FindFirstComponentInScene<QECamera>(NameCameraEditor);
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
    this->_editorCamera.reset();
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

void QESessionManager::UpdateEditorCameraViewportSize(uint32_t width, uint32_t height)
{
    if (this->_editorCamera == nullptr)
        return;

    VkExtent2D extent{};
    extent.width = std::max(1u, width);
    extent.height = std::max(1u, height);

    this->_editorCamera->UpdateViewportSize(extent);
    this->_editorCamera->UpdateCamera();
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

void QESessionManager::SetupEditor()
{
    auto cullingSceneManager = CullingSceneManager::getInstance();

    cullingSceneManager->EnsureInitialized();
    cullingSceneManager->DebugMode = _showCullingAABBDebug;

    auto debugSystem = QEDebugSystem::getInstance();
    debugSystem->SetEnabled(_showColliderDebug);

    if (_isEditor && _setupEditorCallback)
    {
        _setupEditorCallback();
    }
}

void QESessionManager::CleanEditorResources()
{
    if (_cleanEditorResourcesCallback)
    {
        _cleanEditorResourcesCallback();
    }
}

void QESessionManager::ClearEditorBindings()
{
    _extraRenderTarget = nullptr;
    _extraScenePass = {};
    _extraEditorPass = {};
    _setEditorGridVisibilityCallback = {};
    _setupEditorCallback = {};
    _cleanEditorResourcesCallback = {};
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
    if (!IsCameraActiveInHierarchy(_editorCamera))
    {
        _editorCamera = nullptr;
    }

    if (!IsCameraActiveInHierarchy(_gameCamera))
    {
        _gameCamera = nullptr;
    }

    if (_isEditor)
    {
        _activeCamera = _editorCamera ? _editorCamera : _gameCamera;
    }
    else
    {
        _activeCamera = _gameCamera ? _gameCamera : _editorCamera;
    }
}

std::shared_ptr<QECamera> QESessionManager::ActiveCamera() const
{
    if (IsCameraActiveInHierarchy(_activeCamera))
    {
        return _activeCamera;
    }

    if (_isEditor)
    {
        if (IsCameraActiveInHierarchy(_editorCamera))
            return _editorCamera;

        if (IsCameraActiveInHierarchy(_gameCamera))
            return _gameCamera;
    }
    else
    {
        if (IsCameraActiveInHierarchy(_gameCamera))
            return _gameCamera;

        if (IsCameraActiveInHierarchy(_editorCamera))
            return _editorCamera;
    }

    return nullptr;
}

void QESessionManager::ResetSceneState()
{
    _gameCamera.reset();

    if (_isEditor)
    {
        _activeCamera = _editorCamera;
    }
    else
    {
        _activeCamera = nullptr;
    }

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
