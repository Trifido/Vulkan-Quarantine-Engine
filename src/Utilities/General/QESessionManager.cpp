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
    this->_editorCamera = std::make_shared<QECamera>(1280.0f, 720.0f);
    this->cameraUBO = std::make_shared<UniformBufferObject>();
    this->cameraUBO->CreateUniformBuffer(sizeof(CameraUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);
}

void QESessionManager::SetEditorMode(bool value)
{
    this->_isEditor = value;
    this->_activeCamera = (_isEditor || _gameCamera == nullptr) ? _editorCamera : _gameCamera;

    if (value || _gameCamera == nullptr)
    {
        auto gameObjectManager = GameObjectManager::getInstance();
        std::shared_ptr<QEGameObject> cameraObject = std::make_shared<QEGameObject>("CameraEditor");
        cameraObject->AddComponent(this->_editorCamera);
        cameraObject->AddComponent(std::make_shared<QECameraController>());

        auto cameraTransform = cameraObject->GetComponent<QETransform>();
        cameraTransform->SetLocalEulerDegrees(glm::vec3(-45.0f, 0.0f, 0.0f));
        cameraTransform->SetLocalPosition(glm::vec3(0.0f, 10.0f, 10.0f));

        gameObjectManager->AddGameObject(cameraObject);
    }
}

void QESessionManager::SetDebugMode(bool value)
{
    this->_isDebugMode = value;

    auto cullingSceneManager = CullingSceneManager::getInstance();
    cullingSceneManager->DebugMode = value;

    auto debugSystem = QEDebugSystem::getInstance();
    debugSystem->SetEnabled(value);
}

void QESessionManager::RegisterActiveSceneCamera()
{
    this->FindNewSceneCamera();

    if (this->_gameCamera != NULL)
    {
        if (!_isEditor)
        {
            _activeCamera = _gameCamera;
            _activeCamera->QEStart();
        }

        this->_newSceneCamera = false;
    }
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

void QESessionManager::UpdateActiveCameraGPUData()
{
    if (_activeCamera != nullptr)
    {
        auto deviceModule = DeviceModule::getInstance();
        for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
        {
            void* data;
            vkMapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame], 0, sizeof(CameraUniform), 0, &data);
            memcpy(data, static_cast<const void*>(this->_activeCamera->CameraData.get()), sizeof(CameraUniform));
            vkUnmapMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[currentFrame]);
        }
    }
}

void QESessionManager::UpdateViewportSize()
{
    auto swapchainModule = SwapChainModule::getInstance();
    this->_activeCamera->UpdateViewportSize(swapchainModule->swapChainExtent);
    this->_activeCamera->UpdateCamera();
}

void QESessionManager::SetupEditor()
{
    auto cullingSceneManager = CullingSceneManager::getInstance();
    auto editorManager = EditorObjectManager::getInstance();

    cullingSceneManager->InitializeCullingSceneResources();
    cullingSceneManager->DebugMode = _isDebugMode;

    auto debugSystem = QEDebugSystem::getInstance();
    debugSystem->SetEnabled(_isDebugMode);

    if (_isEditor)
    {
        std::shared_ptr<Grid> grid_ptr = std::make_shared<Grid>();
        editorManager->AddEditorObject(grid_ptr, "editor:grid");
        grid_ptr->IsRenderable = _isEditor;
    }
}

void QESessionManager::CleanEditorResources()
{
    auto editorManager = EditorObjectManager::getInstance();
    editorManager->Cleanup();
    editorManager->CleanLastResources();
    editorManager->ResetInstance();
}

void QESessionManager::CleanCameras()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->cameraUBO != nullptr)
        {
            auto deviceModule = DeviceModule::getInstance();
            vkDestroyBuffer(deviceModule->device, this->cameraUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->cameraUBO->uniformBuffersMemory[i], nullptr);
        }
    }
}

void QESessionManager::UpdateCullingScene()
{
    CullingSceneManager::getInstance()->UpdateCullingScene();
}

void QESessionManager::CleanCullingResources()
{
    auto cullingSceneManager = CullingSceneManager::getInstance();
    cullingSceneManager->CleanUp();
    cullingSceneManager->ResetInstance();
}

void QESessionManager::FindNewSceneCamera()
{
    auto gameObjectManager = GameObjectManager::getInstance();

    // Ensure the returned object is of the correct type before assignment
    auto foundCamera = gameObjectManager->FindGameComponentInScene(this->newCameraID);
    this->_gameCamera = std::dynamic_pointer_cast<QECamera>(foundCamera);
}
