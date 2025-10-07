#include "QESessionManager.h"
#include <SwapChainModule.h>
#include <EditorObjectManager.h>
#include <CullingSceneManager.h>
#include <PhysicsModule.h>
#include <Grid.h>

QESessionManager::QESessionManager()
{
    this->_editorCamera = CameraEditor::getInstance();
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

void QESessionManager::RegisterSceneCamera(QECamera* camera)
{
    auto swapchainModule = SwapChainModule::getInstance();
    _gameCamera = camera;
    _gameCamera->UpdateViewportSize(swapchainModule->swapChainExtent);

    if (!_isEditor)
    {
        _activeCamera = _gameCamera;
    }
}

void QESessionManager::CleanCameras()
{
    this->_activeCamera->CleanCameraUBO();
}

void QESessionManager::FreeCameraResources()
{
    delete this->_editorCamera;
    this->_editorCamera = nullptr;
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
    cullingSceneManager->AddFrustumComponent(_activeCamera->frustumComponent);
    cullingSceneManager->DebugMode = _isDebugMode;

    physicsModule->InitializeDebugResources();
    physicsModule->debugDrawer->DebugMode = _isDebugMode;

    // Inicializamos los componentes del editor
    std::shared_ptr<Grid> grid_ptr = std::make_shared<Grid>();
    editorManager->AddEditorObject(grid_ptr, "editor:grid");
    grid_ptr->IsRenderable = _isEditor;
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
