#include "QESessionManager.h"
#include <SwapChainModule.h>

QESessionManager::QESessionManager()
{
    this->_editorCamera = CameraEditor::getInstance();
}

void QESessionManager::SetEditorMode(bool value)
{
    this->_isEditor = value;
    this->_activeCamera = (_isEditor || _gameCamera == nullptr) ? _editorCamera : _gameCamera;
}

void QESessionManager::CleanCameras()
{
    this->_activeCamera->CleanCameraUBO();
}

void QESessionManager::FreeCameraResources()
{
    delete this->_editorCamera;
    this->_editorCamera = nullptr;

    if (this->_gameCamera != nullptr)
    {
        delete this->_gameCamera;
        this->_gameCamera = nullptr;
    }
}

void QESessionManager::UpdateViewportSize()
{
    auto swapchainModule = SwapChainModule::getInstance();
    this->_activeCamera->UpdateViewportSize(swapchainModule->swapChainExtent);
    this->_activeCamera->UpdateCamera();
}
