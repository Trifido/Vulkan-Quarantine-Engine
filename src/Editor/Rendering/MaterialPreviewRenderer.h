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
    void Orbit(float deltaYawDegrees, float deltaPitchDegrees);
    void Zoom(float deltaDistance);

    void Resize(uint32_t width, uint32_t height);
    void Render(VkCommandBuffer& commandBuffer, uint32_t currentFrame);

    bool IsReady() const;
    ImTextureID GetImGuiTexture() const;

    PreviewShape GetPreviewShape() const { return _previewShape; }

private:
    void EnsurePreviewScene();
    void InitializePreviewLightingOverrides();
    void UpdateCameraTransform();
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
    std::shared_ptr<UniformBufferObject> _previewLightUBO;
    std::shared_ptr<UniformBufferObject> _previewLightSSBO;
    std::shared_ptr<UniformBufferObject> _previewLightIndexSSBO;
    std::shared_ptr<UniformBufferObject> _previewLightBinSSBO;
    std::shared_ptr<UniformBufferObject> _previewLightTilesSSBO;

    std::shared_ptr<QEGameObject> _cameraObject;
    std::shared_ptr<QECamera> _camera;
    std::shared_ptr<QEGameObject> _sphereObject;
    std::shared_ptr<QEGameObject> _planeObject;
    std::shared_ptr<QEGameObject> _cubeObject;

    float _orbitYawDegrees = 0.0f;
    float _orbitPitchDegrees = -12.0f;
    float _orbitDistance = 2.75f;
    glm::vec3 _orbitTarget = glm::vec3(0.0f);
    bool _initialized = false;
};
