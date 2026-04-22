#include "MaterialPreviewRenderer.h"

#include <array>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <DeviceModule.h>
#include <RenderPassModule.h>
#include <CommandPoolModule.h>
#include <QueueModule.h>
#include <QESessionManager.h>
#include <QEGameObject.h>
#include <QECamera.h>
#include <QETransform.h>
#include <QEGeometryComponent.h>
#include <QEMeshGenerator.h>
#include <QEMeshRenderer.h>
#include <Material.h>
#include <DeviceModule.h>
#include <Helpers/QEMemoryTrack.h>
#include <QEProjectManager.h>

namespace
{
    constexpr float kPreviewCameraDistance = 2.75f;
    constexpr float kPreviewMinDistance = 0.75f;
    constexpr float kPreviewMaxDistance = 8.0f;
    constexpr float kPreviewMinPitch = -80.0f;
    constexpr float kPreviewMaxPitch = 80.0f;
}

MaterialPreviewRenderer::~MaterialPreviewRenderer()
{
    Cleanup();
}

void MaterialPreviewRenderer::Initialize(
    DeviceModule* deviceModule,
    RenderPassModule* renderPassModule,
    CommandPoolModule* commandPoolModule,
    QueueModule* queueModule)
{
    this->deviceModule = deviceModule;
    this->renderPassModule = renderPassModule;
    this->commandPoolModule = commandPoolModule;
    this->queueModule = queueModule;

    _renderResources.Initialize(deviceModule, renderPassModule, commandPoolModule, queueModule);
    _renderResources.Resize(512, 512);

    EnsurePreviewScene();
    _initialized = true;
}

void MaterialPreviewRenderer::Cleanup()
{
    auto cleanupUBO = [&](const std::shared_ptr<UniformBufferObject>& ubo, const char* tag)
    {
        if (!ubo || !deviceModule)
            return;

        for (size_t i = 0; i < ubo->uniformBuffers.size(); ++i)
        {
            if (ubo->uniformBuffers[i] != VK_NULL_HANDLE)
            {
                QE_DESTROY_BUFFER(deviceModule->device, ubo->uniformBuffers[i], tag);
            }

            if (ubo->uniformBuffersMemory[i] != VK_NULL_HANDLE)
            {
                QE_FREE_MEMORY(deviceModule->device, ubo->uniformBuffersMemory[i], tag);
            }
        }
    };

    if (_renderResources.IsValid())
    {
        _renderResources.Cleanup();
    }

    if (_sphereObject)
    {
        _sphereObject->QEDestroy();
        _sphereObject.reset();
    }

    if (_planeObject)
    {
        _planeObject->QEDestroy();
        _planeObject.reset();
    }

    if (_cubeObject)
    {
        _cubeObject->QEDestroy();
        _cubeObject.reset();
    }

    if (_cameraObject)
    {
        _cameraObject->QEDestroy();
        _cameraObject.reset();
    }

    _camera.reset();
    _material.reset();
    _previewMaterial.reset();

    cleanupUBO(_previewCameraUBO, "MaterialPreviewRenderer::Cleanup");
    cleanupUBO(_previewLightUBO, "MaterialPreviewRenderer::Cleanup");
    cleanupUBO(_previewLightSSBO, "MaterialPreviewRenderer::Cleanup");
    cleanupUBO(_previewLightIndexSSBO, "MaterialPreviewRenderer::Cleanup");
    cleanupUBO(_previewLightBinSSBO, "MaterialPreviewRenderer::Cleanup");
    cleanupUBO(_previewLightTilesSSBO, "MaterialPreviewRenderer::Cleanup");

    _previewCameraUBO.reset();
    _previewLightUBO.reset();
    _previewLightSSBO.reset();
    _previewLightIndexSSBO.reset();
    _previewLightBinSSBO.reset();
    _previewLightTilesSSBO.reset();
    _initialized = false;
}

void MaterialPreviewRenderer::Rebuild()
{
    _renderResources.Rebuild();
}

void MaterialPreviewRenderer::SetMaterial(const std::shared_ptr<QEMaterial>& material)
{
    _material = material;

    if (_material && _material->shader)
    {
        MaterialDto dto = _material->ToDto();
        dto.Name = _material->Name + "_Preview";

        const auto resolvedMaterialPath = QEProjectManager::ResolveProjectPath(dto.FilePath);
        dto.UpdateTexturePaths(resolvedMaterialPath.parent_path());

        _previewMaterial = std::make_shared<QEMaterial>(_material->shader, dto);

        if (_previewMaterial->descriptor)
        {
            _previewMaterial->descriptor->SetCameraOverrideUBO(_previewCameraUBO);
            _previewMaterial->descriptor->SetLightOverrides(
                _previewLightUBO,
                _previewLightSSBO,
                _previewLightIndexSSBO,
                _previewLightBinSSBO,
                _previewLightTilesSSBO);
        }

        _previewMaterial->InitializeMaterialData();
        SyncFromMaterial(_material);
    }
    else
    {
        _previewMaterial.reset();
    }

    for (const auto& object : { _sphereObject, _planeObject, _cubeObject })
    {
        if (object)
        {
            object->SetMaterial(_previewMaterial);
        }
    }
}

void MaterialPreviewRenderer::SyncFromMaterial(const std::shared_ptr<QEMaterial>& material)
{
    if (!material || !_previewMaterial)
        return;

    _previewMaterial->renderQueue = material->renderQueue;

    _previewMaterial->materialData.SetMaterialField("Diffuse", glm::vec3(material->materialData.Diffuse));
    _previewMaterial->materialData.SetMaterialField("Ambient", glm::vec3(material->materialData.Ambient));
    _previewMaterial->materialData.SetMaterialField("Specular", glm::vec3(material->materialData.Specular));
    _previewMaterial->materialData.SetMaterialField("Emissive", glm::vec3(material->materialData.Emissive));
    _previewMaterial->materialData.SetMaterialField("Opacity", material->materialData.Opacity);
    _previewMaterial->materialData.SetMaterialField("Metallic", material->materialData.Metallic);
    _previewMaterial->materialData.SetMaterialField("Roughness", material->materialData.Roughness);
    _previewMaterial->materialData.SetMaterialField("AO", material->materialData.AO);
    _previewMaterial->materialData.SetMaterialField("Clearcoat", material->materialData.Clearcoat);
    _previewMaterial->materialData.SetMaterialField("ClearcoatRoughness", material->materialData.ClearcoatRoughness);
    _previewMaterial->materialData.SetMaterialField("AlphaCutoff", material->materialData.AlphaCutoff);
    _previewMaterial->materialData.SetMaterialField("AlphaMode", static_cast<int>(material->materialData.AlphaMode));
    _previewMaterial->materialData.SetMaterialField("DoubleSided", material->materialData.DoubleSided ? 1 : 0);
    _previewMaterial->UpdateUniformData();
}

void MaterialPreviewRenderer::SetPreviewShape(PreviewShape shape)
{
    _previewShape = shape;
}

void MaterialPreviewRenderer::Orbit(float deltaYawDegrees, float deltaPitchDegrees)
{
    _orbitYawDegrees += deltaYawDegrees;
    _orbitPitchDegrees = std::clamp(_orbitPitchDegrees + deltaPitchDegrees, kPreviewMinPitch, kPreviewMaxPitch);
    UpdateCameraTransform();
}

void MaterialPreviewRenderer::Zoom(float deltaDistance)
{
    _orbitDistance = std::clamp(_orbitDistance + deltaDistance, kPreviewMinDistance, kPreviewMaxDistance);
    UpdateCameraTransform();
}

void MaterialPreviewRenderer::Resize(uint32_t width, uint32_t height)
{
    if (!_initialized)
        return;

    if (_renderResources.NeedsResize(width, height))
    {
        _renderResources.Resize(width, height);
    }

    if (_camera)
    {
        _camera->UpdateViewportSize({ width, height });
        _camera->UpdateCamera();
    }
}

void MaterialPreviewRenderer::Render(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
{
    if (!_initialized || !_material || !_renderResources.IsValid() || !_camera)
        return;

    auto previewObject = GetActivePreviewObject();
    if (!previewObject)
        return;

    auto meshRenderer = previewObject->GetComponent<QEMeshRenderer>();
    if (!meshRenderer)
        return;

    _camera->UpdateCamera();

    void* data = nullptr;
    vkMapMemory(
        deviceModule->device,
        _previewCameraUBO->uniformBuffersMemory[currentFrame],
        0,
        sizeof(UniformCamera),
        0,
        &data);
    memcpy(data, static_cast<const void*>(_camera->CameraData.get()), sizeof(UniformCamera));
    vkUnmapMemory(deviceModule->device, _previewCameraUBO->uniformBuffersMemory[currentFrame]);

    const QERenderTarget& renderTarget = _renderResources.GetRenderTarget();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderTarget.Extent.width);
    viewport.height = static_cast<float>(renderTarget.Extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = renderTarget.Extent;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderTarget.RenderPass;
    renderPassInfo.framebuffer = renderTarget.Framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = renderTarget.Extent;

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = { 0.09f, 0.09f, 0.1f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = { 0.0f, 0.0f, 0.0f, 1.0f };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    meshRenderer->SetDrawCommand(commandBuffer, currentFrame);

    vkCmdEndRenderPass(commandBuffer);
}

bool MaterialPreviewRenderer::IsReady() const
{
    return _initialized && _renderResources.IsValid();
}

ImTextureID MaterialPreviewRenderer::GetImGuiTexture() const
{
    return _renderResources.GetImGuiTexture();
}

void MaterialPreviewRenderer::EnsurePreviewScene()
{
    if (_cameraObject)
        return;

    _cameraObject = std::make_shared<QEGameObject>("MaterialPreviewCamera");
    _cameraObject->AddComponent(std::make_shared<QECamera>(512.0f, 512.0f));
    _camera = _cameraObject->GetComponent<QECamera>();
    _previewCameraUBO = std::make_shared<UniformBufferObject>();
    _previewCameraUBO->CreateUniformBuffer(sizeof(UniformCamera), MAX_FRAMES_IN_FLIGHT, *deviceModule);
    InitializePreviewLightingOverrides();

    if (auto transform = _cameraObject->GetComponent<QETransform>())
    {
        transform->SetLocalPosition(glm::vec3(0.0f, 0.0f, kPreviewCameraDistance));
        transform->SetLocalEulerDegrees(glm::vec3(_orbitPitchDegrees, _orbitYawDegrees, 0.0f));
    }

    _cameraObject->QEStart();
    _cameraObject->QEInit();
    UpdateCameraTransform();

    _sphereObject = CreatePreviewObject(QEPrimitiveType::Sphere, "MaterialPreviewSphere");
    _planeObject = CreatePreviewObject(QEPrimitiveType::Plane, "MaterialPreviewPlane");
    _cubeObject = CreatePreviewObject(QEPrimitiveType::Cube, "MaterialPreviewCube");

    if (auto transform = _planeObject ? _planeObject->GetComponent<QETransform>() : nullptr)
    {
        transform->SetLocalScale(glm::vec3(1.0f));
    }
}

void MaterialPreviewRenderer::InitializePreviewLightingOverrides()
{
    auto lightManager = LightManager::getInstance();
    if (!lightManager)
        return;

    _previewLightUBO = std::make_shared<UniformBufferObject>();
    _previewLightUBO->CreateUniformBuffer(sizeof(LightManagerUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    _previewLightSSBO = std::make_shared<UniformBufferObject>();
    _previewLightSSBO->CreateSSBO(lightManager->lightSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    _previewLightIndexSSBO = std::make_shared<UniformBufferObject>();
    _previewLightIndexSSBO->CreateSSBO(lightManager->lightIndexSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    _previewLightBinSSBO = std::make_shared<UniformBufferObject>();
    _previewLightBinSSBO->CreateSSBO(lightManager->lightBinSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    _previewLightTilesSSBO = std::make_shared<UniformBufferObject>();
    _previewLightTilesSSBO->CreateSSBO(lightManager->lightTilesSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    LightManagerUniform zeroLightData{};
    zeroLightData.numLights = 0;

    std::vector<uint8_t> zeroLights(lightManager->lightSSBOSize, 0);
    std::vector<uint8_t> zeroIndices(lightManager->lightIndexSSBOSize, 0);
    std::vector<uint32_t> zeroBins(lightManager->lightBinSSBOSize / sizeof(uint32_t), 1u);
    std::vector<uint8_t> zeroTiles(lightManager->lightTilesSSBOSize, 0);

    for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
    {
        void* data = nullptr;
        vkMapMemory(deviceModule->device, _previewLightUBO->uniformBuffersMemory[frame], 0, sizeof(LightManagerUniform), 0, &data);
        memcpy(data, &zeroLightData, sizeof(LightManagerUniform));
        vkUnmapMemory(deviceModule->device, _previewLightUBO->uniformBuffersMemory[frame]);

        vkMapMemory(deviceModule->device, _previewLightSSBO->uniformBuffersMemory[frame], 0, lightManager->lightSSBOSize, 0, &data);
        memcpy(data, zeroLights.data(), zeroLights.size());
        vkUnmapMemory(deviceModule->device, _previewLightSSBO->uniformBuffersMemory[frame]);

        vkMapMemory(deviceModule->device, _previewLightIndexSSBO->uniformBuffersMemory[frame], 0, lightManager->lightIndexSSBOSize, 0, &data);
        memcpy(data, zeroIndices.data(), zeroIndices.size());
        vkUnmapMemory(deviceModule->device, _previewLightIndexSSBO->uniformBuffersMemory[frame]);

        vkMapMemory(deviceModule->device, _previewLightBinSSBO->uniformBuffersMemory[frame], 0, lightManager->lightBinSSBOSize, 0, &data);
        memcpy(data, zeroBins.data(), lightManager->lightBinSSBOSize);
        vkUnmapMemory(deviceModule->device, _previewLightBinSSBO->uniformBuffersMemory[frame]);

        vkMapMemory(deviceModule->device, _previewLightTilesSSBO->uniformBuffersMemory[frame], 0, lightManager->lightTilesSSBOSize, 0, &data);
        memcpy(data, zeroTiles.data(), zeroTiles.size());
        vkUnmapMemory(deviceModule->device, _previewLightTilesSSBO->uniformBuffersMemory[frame]);
    }
}

void MaterialPreviewRenderer::UpdateCameraTransform()
{
    if (!_cameraObject)
        return;

    const float yaw = glm::radians(_orbitYawDegrees);
    const float pitch = glm::radians(_orbitPitchDegrees);

    glm::vec3 offset{};
    offset.x = _orbitDistance * cosf(pitch) * sinf(yaw);
    offset.y = _orbitDistance * sinf(-pitch);
    offset.z = _orbitDistance * cosf(pitch) * cosf(yaw);

    if (auto transform = _cameraObject->GetComponent<QETransform>())
    {
        const glm::vec3 cameraPosition = _orbitTarget + offset;
        const glm::mat4 view = glm::lookAtRH(cameraPosition, _orbitTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat worldRotation = glm::normalize(glm::quat_cast(glm::inverse(glm::mat3(view))));

        transform->SetLocalPosition(cameraPosition);
        transform->SetLocalRotation(worldRotation);
    }
}

std::shared_ptr<QEGameObject> MaterialPreviewRenderer::CreatePreviewObject(QEPrimitiveType primitiveType, const char* name) const
{
    auto object = std::make_shared<QEGameObject>(name);

    std::unique_ptr<IQEMeshGenerator> generator;
    switch (primitiveType)
    {
    case QEPrimitiveType::Sphere:
        generator = std::make_unique<SphereGenerator>();
        break;
    case QEPrimitiveType::Plane:
        generator = std::make_unique<FloorGenerator>();
        break;
    case QEPrimitiveType::Cube:
        generator = std::make_unique<CubeGenerator>();
        break;
    default:
        return nullptr;
    }

    object->AddComponent(std::make_shared<QEGeometryComponent>(std::move(generator)));
    object->AddComponent(std::make_shared<QEMeshRenderer>());

    if (_previewMaterial)
    {
        object->SetMaterial(_previewMaterial);
    }

    object->QEStart();
    object->QEInit();
    return object;
}

std::shared_ptr<QEGameObject> MaterialPreviewRenderer::GetActivePreviewObject() const
{
    switch (_previewShape)
    {
    case PreviewShape::Plane:
        return _planeObject;
    case PreviewShape::Cube:
        return _cubeObject;
    case PreviewShape::Sphere:
    default:
        return _sphereObject;
    }
}
