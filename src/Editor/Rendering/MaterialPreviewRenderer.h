#pragma once

#include <memory>

#include <vulkan/vulkan.h>
#include <imgui.h>

#include <Editor/Rendering/EditorViewportResources.h>
#include <Editor/Core/EditorSceneObjectFactory.h>
#include <UBO.h>

class DeviceModule;
class RenderPassModule;
class CommandPoolModule;
class QueueModule;
class QEMaterial;
class QEGameObject;
class QECamera;

class MaterialPreviewRenderer
{
public:
    enum class PreviewShape
    {
        Sphere = 0,
        Plane,
        Cube
    };

    MaterialPreviewRenderer() = default;
    ~MaterialPreviewRenderer();

    void Initialize(
        DeviceModule* deviceModule,
        RenderPassModule* renderPassModule,
        CommandPoolModule* commandPoolModule,
        QueueModule* queueModule);
    void Cleanup();
    void Rebuild();

    void SetMaterial(const std::shared_ptr<QEMaterial>& material);
    void SyncFromMaterial(const std::shared_ptr<QEMaterial>& material);
    void SetPreviewShape(PreviewShape shape);

    void Resize(uint32_t width, uint32_t height);
    void Render(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

    bool IsReady() const;
    ImTextureID GetImGuiTexture() const;

    PreviewShape GetPreviewShape() const { return _previewShape; }

private:
    void EnsurePreviewScene();
    std::shared_ptr<QEGameObject> CreatePreviewObject(QEPrimitiveType primitiveType, const char* name) const;
    std::shared_ptr<QEGameObject> GetActivePreviewObject() const;

private:
    DeviceModule* deviceModule = nullptr;
    RenderPassModule* renderPassModule = nullptr;
    CommandPoolModule* commandPoolModule = nullptr;
    QueueModule* queueModule = nullptr;

    EditorViewportResources _renderResources;
    std::shared_ptr<QEMaterial> _material;
    std::shared_ptr<QEMaterial> _previewMaterial;
    PreviewShape _previewShape = PreviewShape::Sphere;
    std::shared_ptr<UniformBufferObject> _previewCameraUBO;

    std::shared_ptr<QEGameObject> _cameraObject;
    std::shared_ptr<QECamera> _camera;
    std::shared_ptr<QEGameObject> _sphereObject;
    std::shared_ptr<QEGameObject> _planeObject;
    std::shared_ptr<QEGameObject> _cubeObject;

    bool _initialized = false;
};
