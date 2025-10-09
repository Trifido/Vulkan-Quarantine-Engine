#include "QESessionManager.h"
#include <SwapChainModule.h>
#include <GameObjectManager.h>
#include <EditorObjectManager.h>
#include <CullingSceneManager.h>
#include <PhysicsModule.h>
#include <Grid.h>

QESessionManager::QESessionManager()
{
    this->_editorCamera = std::make_shared<QECamera>(1280.0f, 720.0f, CameraDto());
}

void QESessionManager::SetEditorMode(bool value)
{
    this->_isEditor = value;
    this->_activeCamera = (_isEditor || _gameCamera == nullptr) ? _editorCamera : _gameCamera;
}

void QESessionManager::SetDebugMode(bool value)
{
    this->_isDebugMode = value;

    auto cullingSceneManager = CullingSceneManager::getInstance();
    cullingSceneManager->DebugMode = value;

    auto physicsModule = PhysicsModule::getInstance();
    physicsModule->debugDrawer->DebugMode = value;
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

void QESessionManager::CleanCameras()
{
    this->_activeCamera->CleanCameraUBO();
}

void QESessionManager::FreeCameraResources()
{
    this->_editorCamera.reset();
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
    auto physicsModule = PhysicsModule::getInstance();
    auto editorManager = EditorObjectManager::getInstance();

    cullingSceneManager->InitializeCullingSceneResources();
    cullingSceneManager->DebugMode = _isDebugMode;

    physicsModule->InitializeDebugResources();
    physicsModule->debugDrawer->DebugMode = _isDebugMode;

    // Inicializamos los componentes del editor
    _isEditor = true;
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
