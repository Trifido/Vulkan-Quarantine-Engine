#include "QESessionManager.h"
#include <SwapChainModule.h>
#include <GameObjectManager.h>
#include <EditorObjectManager.h>
#include <CullingSceneManager.h>
#include <Grid.h>
#include <SynchronizationModule.h>
#include <QECameraController.h>
#include <DebugSystem/QEDebugSystem.h>

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

void QESessionManager::SetDebugMode(bool value)
{
    _isDebugMode = value;
    SetShowColliderDebug(value);
    SetShowCullingAABBDebug(value);
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
    if (_activeCamera == nullptr)
        return;

    if (this->cameraUBO == nullptr)
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
        static_cast<const void*>(this->_activeCamera->CameraData.get()),
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
    auto editorManager = EditorObjectManager::getInstance();

    cullingSceneManager->EnsureInitialized();
    cullingSceneManager->DebugMode = _showCullingAABBDebug;

    auto debugSystem = QEDebugSystem::getInstance();
    debugSystem->SetEnabled(_showColliderDebug);

    if (_isEditor)
    {
        auto existing = editorManager->GetObject("editor:grid");

        if (!existing)
        {
            std::shared_ptr<Grid> grid_ptr = std::make_shared<Grid>();
            editorManager->AddEditorObject(grid_ptr, "editor:grid");
            grid_ptr->IsRenderable = true;
        }
        else
        {
            existing->IsRenderable = true;
        }
    }
}

void QESessionManager::CleanEditorResources()
{
    auto editorManager = EditorObjectManager::getInstance();
    editorManager->Cleanup();
    editorManager->CleanLastResources();
    editorManager->ResetInstance();
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
    if (_isEditor)
    {
        _activeCamera = _editorCamera ? _editorCamera : _gameCamera;
    }
    else
    {
        _activeCamera = _gameCamera ? _gameCamera : _editorCamera;
    }
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
        vkDestroyBuffer(deviceModule->device, this->cameraUBO->uniformBuffers[i], nullptr);
        vkFreeMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], nullptr);
    }

    this->cameraUBO.reset();
}
