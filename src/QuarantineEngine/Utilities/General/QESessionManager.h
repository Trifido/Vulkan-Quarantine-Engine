#pragma once

#include <memory>
#include <string>
#include <functional>
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

    void SetEditorMode(bool value);

    void SetShowColliderDebug(bool value);
    void SetShowCullingAABBDebug(bool value);
    void SetShowEditorGrid(bool value);

    bool ShowColliderDebug() const { return _showColliderDebug; }
    bool ShowCullingAABBDebug() const { return _showCullingAABBDebug; }
    bool ShowEditorGrid() const { return _showEditorGrid; }

    void RegisterActiveSceneCamera();
    void RegisterSceneCameras();
    void SetFindNewSceneCamera(std::string cameraID);
    void FreeCameraResources();

    void UpdateActiveCameraGPUData(uint32_t currentFrame);
    void UpdateCameraGPUData(const std::shared_ptr<QECamera>& camera, uint32_t currentFrame);

    void UpdateEditorCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateGameCameraViewportSize(uint32_t width, uint32_t height);
    void UpdateActiveCameraViewportSize(uint32_t width, uint32_t height);

    void SetupEditor();
    void CleanEditorResources();

    void UpdateCullingScene();
    void CleanCullingResources();

    void FindNewSceneCamera();
    void ResolveActiveCamera();

    void ResetSceneState();
    void ShutdownPersistentResources();

    std::shared_ptr<UniformBufferObject> GetCameraUBO() { return this->cameraUBO; }

    std::shared_ptr<QECamera> ActiveCamera() const;
    std::shared_ptr<QECamera> EditorCamera() const { return _editorCamera; }
    std::shared_ptr<QECamera> GameCamera() const { return _gameCamera; }

    bool IsEditor() const { return _isEditor; }

    const QERenderTarget* GetExtraRenderTarget() const { return _extraRenderTarget; }
    void SetExtraRenderTarget(const QERenderTarget* renderTarget) { _extraRenderTarget = renderTarget; }

    const std::function<void(VkCommandBuffer&, uint32_t)>& GetExtraScenePass() const { return _extraScenePass; }
    void SetExtraScenePass(std::function<void(VkCommandBuffer&, uint32_t)> callback) { _extraScenePass = std::move(callback); }

    const std::function<void(VkCommandBuffer&, uint32_t)>& GetExtraEditorPass() const { return _extraEditorPass; }
    void SetExtraEditorPass(std::function<void(VkCommandBuffer&, uint32_t)> callback) { _extraEditorPass = std::move(callback); }

    void SetEditorGridVisibilityHandler(std::function<void(bool)> callback) { _setEditorGridVisibilityCallback = std::move(callback); }
    void SetSetupEditorHandler(std::function<void()> callback) { _setupEditorCallback = std::move(callback); }
    void SetCleanEditorResourcesHandler(std::function<void()> callback) { _cleanEditorResourcesCallback = std::move(callback); }
    void ClearEditorBindings();

private:
    static constexpr const char* NameCameraEditor = "QECameraEditor";
    bool IsCameraActiveInHierarchy(const std::shared_ptr<QECamera>& camera) const;

    bool _isEditor = false;
    bool _newSceneCamera = false;

    bool _showColliderDebug = false;
    bool _showCullingAABBDebug = false;
    bool _showEditorGrid = true;

    std::string newCameraID;

    std::shared_ptr<QECamera> _activeCamera = nullptr;
    std::shared_ptr<QECamera> _editorCamera = nullptr;
    std::shared_ptr<QECamera> _gameCamera = nullptr;
    const QERenderTarget* _extraRenderTarget = nullptr;
    std::function<void(VkCommandBuffer&, uint32_t)> _extraScenePass;
    std::function<void(VkCommandBuffer&, uint32_t)> _extraEditorPass;
    std::function<void(bool)> _setEditorGridVisibilityCallback;
    std::function<void()> _setupEditorCallback;
    std::function<void()> _cleanEditorResourcesCallback;
    std::shared_ptr<UniformBufferObject> cameraUBO = nullptr;
};


namespace QE
{
    using ::QECamera;
    using ::UniformBufferObject;
    using ::QESessionManager;
} // namespace QE
// QE namespace aliases
