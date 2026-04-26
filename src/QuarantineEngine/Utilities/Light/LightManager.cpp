#include "LightManager.h"
#include <algorithm>
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include <SynchronizationModule.h>
#include <SunLight.h>
#include <QECameraContext.h>
#include <QEGameObject.h>
#include "QECamera.h"
#include <Helpers/QEMemoryTrack.h>

bool compareDistance(const LightMap& a, const LightMap& b)
{
    //if (a.projected_z < b.projected_z) return -1;
    //if (a.projected_z > b.projected_z) return 1;
    return a.projected_z < b.projected_z;
}

namespace
{
bool IsLightOwnerInactiveInHierarchy(const std::shared_ptr<QELight>& light)
{
    if (!light || !light->Owner)
        return false;

    return !light->Owner->IsActiveInHierarchy();
}
}

LightManager::LightManager()
{
    this->deviceModule = DeviceModule::getInstance();
    this->swapChainModule = SwapChainModule::getInstance();
    this->renderPassModule = RenderPassModule::getInstance();

    this->lightManagerUniform = std::make_shared<LightManagerUniform>();
    this->lightUBO = std::make_shared<UniformBufferObject>();
    this->lightUBO->CreateUniformBuffer(sizeof(LightManagerUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightSSBOSize = sizeof(LightUniform) * this->MAX_NUM_LIGHT;
    this->lightSSBO = std::make_shared<UniformBufferObject>();
    this->lightSSBO->CreateSSBO(this->lightSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightIndexSSBOSize = sizeof(uint32_t) * this->MAX_NUM_LIGHT;
    this->lightIndexSSBO = std::make_shared<UniformBufferObject>();
    this->lightIndexSSBO->CreateSSBO(this->lightIndexSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightTilesSSBOSize = sizeof(uint32_t) * this->MAX_NUM_TILES;
    this->lightTilesSSBO = std::make_shared<UniformBufferObject>();
    this->lightTilesSSBO->CreateSSBO(this->lightTilesSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightBinSSBOSize = sizeof(uint32_t) * this->BIN_SLICES;
    this->lightBinSSBO = std::make_shared<UniformBufferObject>();
    this->lightBinSSBO->CreateSSBO(this->lightBinSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightBuffer.reserve(this->MAX_NUM_LIGHT);

    this->PointShadowDescritors = std::make_shared<PointShadowDescriptorsManager>();
    this->CSMDescritors = std::make_shared<CSMDescriptorsManager>();
    this->SpotShadowDescritors = std::make_shared<SpotShadowDescriptorsManager>();
}

void LightManager::AddDirShadowMapShader(std::shared_ptr<ShaderModule> shadow_mapping_shader)
{
    this->CSMShaderModule = shadow_mapping_shader;
    this->CSMPipelineModule = this->CSMShaderModule->ShadowPipelineModule;
}

void LightManager::AddOmniShadowMapShader(std::shared_ptr<ShaderModule> omni_shadow_mapping_shader)
{
    this->OmniShadowShaderModule = omni_shadow_mapping_shader;
    this->OmniShadowPipelineModule = this->OmniShadowShaderModule->ShadowPipelineModule;
}

void LightManager::AddNewLight(std::shared_ptr<QELight> light_ptr, std::string& name)
{
    if (light_ptr->ResourcesInitialized)
    {
        return;
    }

    switch (light_ptr->lightType)
    {
        default:
        case LightType::POINT_LIGHT:
            this->PointLights.push_back(std::static_pointer_cast<QEPointLight>(light_ptr));
            this->PointLights.back()->Setup(this->renderPassModule->OmniShadowMappingRenderPass);
            this->PointLights.back()->idxShadowMap = (uint32_t)this->PointLights.size() - 1;

            this->AddLight(light_ptr, name);
            this->PointShadowDescritors->AddPointLightResources(
                this->PointLights.back()->shadowMappingResourcesPtr->shadowMapUBO,
                this->PointLights.back()->shadowMappingResourcesPtr->CubemapImageView,
                this->PointLights.back()->shadowMappingResourcesPtr->CubemapSampler);
            break;

        case LightType::SUN_LIGHT:
        case LightType::DIRECTIONAL_LIGHT:
            if (light_ptr->lightType == LightType::SUN_LIGHT)
            {
                this->DirLights.push_back(std::static_pointer_cast<QESunLight>(light_ptr));
                this->DirLights.back()->Setup(this->renderPassModule->DirShadowMappingRenderPass);
            }
            else
            {
                this->DirLights.push_back(std::static_pointer_cast<QEDirectionalLight>(light_ptr));
                this->DirLights.back()->Setup(this->renderPassModule->DirShadowMappingRenderPass);
            }
            this->DirLights.back()->idxShadowMap = (uint32_t)this->DirLights.size() - 1;

            this->AddLight(light_ptr, name);
            this->CSMDescritors->AddDirLightResources(
                this->DirLights.back()->shadowMappingResourcesPtr->OffscreenShadowMapUBO,
                this->DirLights.back()->shadowMappingResourcesPtr->CSMImageView,
                this->DirLights.back()->shadowMappingResourcesPtr->CSMSampler);

            this->CSMDescritors->BindResources(this->DirLights.back()->shadowMappingResourcesPtr->CascadeResourcesPtr);

            SyncDirectionalLightIndices();
            break;

        case LightType::SPOT_LIGHT:
            this->SpotLights.push_back(std::dynamic_pointer_cast<QESpotLight>(light_ptr));
            this->SpotLights.back()->Setup(this->renderPassModule->DirShadowMappingRenderPass);
            this->SpotLights.back()->idxShadowMap = (uint32_t)this->SpotLights.size() - 1;
            this->AddLight(light_ptr, name);

            this->SpotShadowDescritors->AddSpotLightResources(
                this->SpotLights.back()->shadowMappingResourcesPtr->OffscreenShadowMapUBO,
                this->SpotLights.back()->shadowMappingResourcesPtr->ShadowImageView,
                this->SpotLights.back()->shadowMappingResourcesPtr->ShadowSampler);
            this->SpotShadowDescritors->BindResources(this->SpotLights.back()->shadowMappingResourcesPtr);
            break;
    }

    light_ptr->ResourcesInitialized = true;
}

std::shared_ptr<QELight> LightManager::CreateLight(LightType type, std::string name)
{
    std::shared_ptr<QELight> newLight;

    switch (type)
    {
        default:
        case LightType::POINT_LIGHT:
            newLight = std::make_shared<QEPointLight>();
            break;

        case LightType::SUN_LIGHT:
            newLight = std::make_shared<QESunLight>();
            break;
        case LightType::DIRECTIONAL_LIGHT:
            newLight = std::make_shared<QEDirectionalLight>();      
            break;

        case LightType::SPOT_LIGHT:
            newLight = std::make_shared<QESpotLight>();
            break;
    }

    newLight->Name = name;
    return newLight;
}

void LightManager::DeleteLight(std::shared_ptr<QELight> light_ptr, std::string& name)
{
    if (!light_ptr)
        return;

    switch (light_ptr->lightType)
    {
    default:
    case LightType::POINT_LIGHT:
    {
        auto it = std::find_if(PointLights.begin(), PointLights.end(),
            [light_ptr](const auto& light) { return light && light->id == light_ptr->id; });

        if (it != PointLights.end())
        {
            int localLightPosition = static_cast<int>(std::distance(PointLights.begin(), it));
            PointLights.erase(it);

            if (PointShadowDescritors)
            {
                PointShadowDescritors->DeletePointLightResources(localLightPosition);
            }
        }
    }
    break;

    case LightType::DIRECTIONAL_LIGHT:
    case LightType::SUN_LIGHT:
    {
        if (light_ptr->lightType == LightType::SUN_LIGHT)
        {
            auto sunLight = std::dynamic_pointer_cast<QESunLight>(light_ptr);
            if (sunLight)
            {
                sunLight->CleanupSunResources();
            }
        }
        else
        {
            auto directionalLight = std::dynamic_pointer_cast<QEDirectionalLight>(light_ptr);
            if (directionalLight)
            {
                directionalLight->CleanShadowMapResources();
            }
        }

        auto it = std::find_if(DirLights.begin(), DirLights.end(),
            [light_ptr](const auto& light) { return light && light->id == light_ptr->id; });

        if (it != DirLights.end())
        {
            int localLightPosition = static_cast<int>(std::distance(DirLights.begin(), it));
            DirLights.erase(it);

            if (CSMDescritors)
            {
                CSMDescritors->DeleteDirLightResources(localLightPosition);

                ReindexShadowMaps();
                SyncDirectionalLightIndices();
            }
        }
    }
    break;

    case LightType::SPOT_LIGHT:
    {
        auto it = std::find_if(SpotLights.begin(), SpotLights.end(),
            [light_ptr](const auto& light) { return light && light->id == light_ptr->id; });

        if (it != SpotLights.end())
        {
            int localLightPosition = static_cast<int>(std::distance(SpotLights.begin(), it));
            SpotLights.erase(it);

            if (SpotShadowDescritors)
            {
                SpotShadowDescritors->DeleteSpotLightResources(localLightPosition);
                ReindexShadowMaps();
            }
        }
    }
    break;
    }

    auto it = std::find_if(_lights.begin(), _lights.end(),
        [light_ptr](const auto& light) { return light.second && light.second->id == light_ptr->id; });

    if (it != _lights.end())
    {
        _lights.erase(it);
    }

    lightBuffer.clear();
    lightBuffer.reserve(MAX_NUM_LIGHT);

    for (auto& entry : _lights)
    {
        if (entry.second && entry.second->uniform)
        {
            lightBuffer.push_back(*entry.second->uniform);
        }
    }

    currentNumLights = static_cast<uint32_t>(_lights.size());
}

void LightManager::DeleteLightByName(const std::string& name)
{
    auto it = _lights.find(name);
    if (it == _lights.end() || !it->second)
        return;

    std::string mutableName = it->first;
    DeleteLight(it->second, mutableName);
}

bool LightManager::RenameLight(const std::string& oldName, const std::string& newName, const std::shared_ptr<QELight>& light_ptr)
{
    if (newName.empty())
        return false;

    std::shared_ptr<QELight> resolvedLight = light_ptr;
    auto oldIt = _lights.end();

    if (!oldName.empty())
    {
        oldIt = _lights.find(oldName);
        if (oldIt != _lights.end())
        {
            resolvedLight = oldIt->second;
        }
    }

    if (!resolvedLight)
    {
        for (auto it = _lights.begin(); it != _lights.end(); ++it)
        {
            if (it->second == light_ptr)
            {
                oldIt = it;
                resolvedLight = it->second;
                break;
            }
        }
    }

    if (!resolvedLight)
        return false;

    auto newIt = _lights.find(newName);
    if (newIt != _lights.end() && newIt->second != resolvedLight)
        return false;

    if (oldIt != _lights.end() && oldIt->second == resolvedLight && oldIt->first != newName)
    {
        _lights.erase(oldIt);
    }

    _lights[newName] = resolvedLight;
    resolvedLight->Name = newName;
    return true;
}

std::vector<LightDto> LightManager::GetLightDtos(std::ifstream& file)
{
    std::vector<LightDto> lightDtos;

    int numLights;
    file.read(reinterpret_cast<char*>(&numLights), sizeof(int));

    for (int i = 0; i < numLights; i++)
    {
        LightDto lightDto;

        int nameLength;
        file.read(reinterpret_cast<char*>(&nameLength), sizeof(int));
        lightDto.name.resize(nameLength);
        file.read(&lightDto.name[0], nameLength);

        file.read(reinterpret_cast<char*>(&lightDto.lightType), sizeof(LightType));
        file.read(reinterpret_cast<char*>(&lightDto.radius), sizeof(float));
        file.read(reinterpret_cast<char*>(&lightDto.worldTransform), sizeof(glm::mat4));
        file.read(reinterpret_cast<char*>(&lightDto.diffuse), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&lightDto.specular), sizeof(glm::vec3));
        file.read(reinterpret_cast<char*>(&lightDto.cutOff), sizeof(float));
        file.read(reinterpret_cast<char*>(&lightDto.outerCutoff), sizeof(float));

        lightDtos.push_back(lightDto);
    }

    return lightDtos;
}

void LightManager::SaveLights(std::ofstream& file)
{
    int numLights = static_cast<int>(this->_lights.size());
    file.write(reinterpret_cast<const char*>(&numLights), sizeof(int));

    for (auto& it : this->_lights)
    {
        auto light = it.second;

        std::string name = it.first;
        int nameLength = static_cast<int>(name.length());
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(int));
        file.write(name.c_str(), nameLength);
        file.write(reinterpret_cast<const char*>(&light->lightType), sizeof(LightType));

        float radius = light->GetDistanceEffect();
        file.write(reinterpret_cast<const char*>(&radius), sizeof(float));
        file.write(reinterpret_cast<const char*>(&light->transform->GetWorldMatrix()), sizeof(glm::mat4));
        file.write(reinterpret_cast<const char*>(&light->diffuse), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&light->specular), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&light->cutOff), sizeof(float));
        file.write(reinterpret_cast<const char*>(&light->outerCutoff), sizeof(float));
    }
}

std::shared_ptr<QELight> LightManager::GetLight(std::string name)
{
    auto it =_lights.find(name);

    if (it != _lights.end())
        return it->second;

    return nullptr;
}

const std::shared_ptr<UniformBufferObject>& LightManager::GetLightUBO() const
{
    return lightUBO;
}

const std::shared_ptr<UniformBufferObject>& LightManager::GetLightSSBO() const
{
    return lightSSBO;
}

VkDeviceSize LightManager::GetLightSSBOSize() const
{
    return lightSSBOSize;
}

const std::shared_ptr<UniformBufferObject>& LightManager::GetLightIndexSSBO() const
{
    return lightIndexSSBO;
}

VkDeviceSize LightManager::GetLightIndexSSBOSize() const
{
    return lightIndexSSBOSize;
}

const std::shared_ptr<UniformBufferObject>& LightManager::GetLightTilesSSBO() const
{
    return lightTilesSSBO;
}

VkDeviceSize LightManager::GetLightTilesSSBOSize() const
{
    return lightTilesSSBOSize;
}

const std::shared_ptr<UniformBufferObject>& LightManager::GetLightBinSSBO() const
{
    return lightBinSSBO;
}

VkDeviceSize LightManager::GetLightBinSSBOSize() const
{
    return lightBinSSBOSize;
}

const std::vector<std::shared_ptr<QEDirectionalLight>>& LightManager::GetDirectionalLights() const
{
    return DirLights;
}

const std::vector<std::shared_ptr<QESpotLight>>& LightManager::GetSpotLights() const
{
    return SpotLights;
}

const std::vector<std::shared_ptr<QEPointLight>>& LightManager::GetPointLights() const
{
    return PointLights;
}

const std::shared_ptr<PointShadowDescriptorsManager>& LightManager::GetPointShadowDescriptors() const
{
    return PointShadowDescritors;
}

const std::shared_ptr<CSMDescriptorsManager>& LightManager::GetCSMDescriptors() const
{
    return CSMDescritors;
}

const std::shared_ptr<SpotShadowDescriptorsManager>& LightManager::GetSpotShadowDescriptors() const
{
    return SpotShadowDescritors;
}

const std::shared_ptr<ShadowPipelineModule>& LightManager::GetCSMPipelineModule() const
{
    return CSMPipelineModule;
}

const std::shared_ptr<ShadowPipelineModule>& LightManager::GetOmniShadowPipelineModule() const
{
    return OmniShadowPipelineModule;
}

const std::shared_ptr<ShaderModule>& LightManager::GetCSMShaderModule() const
{
    return CSMShaderModule;
}

const std::shared_ptr<ShaderModule>& LightManager::GetOmniShadowShaderModule() const
{
    return OmniShadowShaderModule;
}

void LightManager::InitializeShadowMaps()
{
    this->PointShadowDescritors->InitializeDescriptorSetLayouts(this->OmniShadowShaderModule);
    this->CSMDescritors->InitializeDescriptorSetLayouts(this->CSMShaderModule);
    this->SpotShadowDescritors->InitializeDescriptorSetLayouts(this->CSMShaderModule);
}

void LightManager::UpdateUniform()
{
    this->lightManagerUniform->numLights = (uint32_t)std::min(this->_lights.size(), this->MAX_NUM_LIGHT);

    lightBuffer.clear();
    lightBuffer.reserve(MAX_NUM_LIGHT);

    for (auto& it : this->_lights)
    {
        if (!it.second)
            continue;

        it.second->UpdateUniform();

        if (it.second->uniform)
        {
            auto lightUniform = *it.second->uniform;

            if (IsLightOwnerInactiveInHierarchy(it.second))
            {
                lightUniform.diffuse = glm::vec3(0.0f);
                lightUniform.specular = glm::vec3(0.0f);
            }

            lightBuffer.push_back(lightUniform);
        }
    }

    this->UpdateUBOLight();
}

void LightManager::UpdateUBOLight()
{
    const size_t lightCount = lightBuffer.size();
    const size_t uploadSize = lightCount * sizeof(LightUniform);

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data = nullptr;
        vkMapMemory(this->deviceModule->device, this->lightUBO->uniformBuffersMemory[currentFrame], 0, sizeof(LightManagerUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->lightManagerUniform.get()), sizeof(LightManagerUniform));
        vkUnmapMemory(this->deviceModule->device, this->lightUBO->uniformBuffersMemory[currentFrame]);

        void* data2 = nullptr;
        vkMapMemory(this->deviceModule->device, this->lightSSBO->uniformBuffersMemory[currentFrame], 0, this->lightSSBOSize, 0, &data2);

        if (uploadSize > 0 && !lightBuffer.empty())
        {
            memcpy(data2, lightBuffer.data(), uploadSize);
        }

        vkUnmapMemory(this->deviceModule->device, this->lightSSBO->uniformBuffersMemory[currentFrame]);
    }
}

void LightManager::UpdateCSMLights()
{
    for (auto& light : this->DirLights)
    {
        if (!light)
        {
            QE_LOG_ERROR_CAT_F("LightManager", "DirLights contains null light");
            continue;
        }

        light->EnsureRuntimeState();

        if (!light->transform)
        {
            QE_LOG_ERROR_CAT_F("LightManager", "Directional light '{}' has null transform", light->Name);
            continue;
        }

        if (!light->shadowMappingResourcesPtr)
        {
            QE_LOG_ERROR_CAT_F("LightManager", "Directional light '{}' has null shadowMappingResourcesPtr", light->Name);
            continue;
        }

        light->UpdateUniform();
    }
}

void LightManager::ShutdownPersistentResources()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->lightUBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->lightUBO->uniformBuffers[i], "LightManager::ShutdownPersistentResources");
            QE_FREE_MEMORY(deviceModule->device, this->lightUBO->uniformBuffersMemory[i], "LightManager::ShutdownPersistentResources");
        }

        if (this->lightSSBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->lightSSBO->uniformBuffers[i], "LightManager::ShutdownPersistentResources");
            QE_FREE_MEMORY(deviceModule->device, this->lightSSBO->uniformBuffersMemory[i], "LightManager::ShutdownPersistentResources");
        }

        if (this->lightIndexSSBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->lightIndexSSBO->uniformBuffers[i], "LightManager::ShutdownPersistentResources");
            QE_FREE_MEMORY(deviceModule->device, this->lightIndexSSBO->uniformBuffersMemory[i], "LightManager::ShutdownPersistentResources");
        }

        if (this->lightTilesSSBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->lightTilesSSBO->uniformBuffers[i], "LightManager::ShutdownPersistentResources");
            QE_FREE_MEMORY(deviceModule->device, this->lightTilesSSBO->uniformBuffersMemory[i], "LightManager::ShutdownPersistentResources");
        }

        if (this->lightBinSSBO)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->lightBinSSBO->uniformBuffers[i], "LightManager::ShutdownPersistentResources");
            QE_FREE_MEMORY(deviceModule->device, this->lightBinSSBO->uniformBuffersMemory[i], "LightManager::ShutdownPersistentResources");
        }
    }
}

void LightManager::ResetSceneState()
{
    ResetShadowSceneState();

    _lights.clear();
    DirLights.clear();
    SpotLights.clear();
    PointLights.clear();

    lightBuffer.clear();
    sortedLight.clear();
    lights_bin.clear();
    lights_index.clear();
    light_tiles_bits.clear();

    currentNumLights = 0;
}

void LightManager::ResetShadowSceneState()
{
    for (auto& pLight : this->PointLights)
    {
        if (pLight)
            pLight->CleanShadowMapResources();
    }

    for (auto& dLight : this->DirLights)
    {
        if (dLight)
            dLight->CleanShadowMapResources();
    }

    for (auto& sLight : this->SpotLights)
    {
        if (sLight)
            sLight->CleanShadowMapResources();
    }

    if (this->PointShadowDescritors)
    {
        this->PointShadowDescritors->ResetSceneState();
    }

    if (this->CSMDescritors)
    {
        this->CSMDescritors->ResetSceneState();
    }

    if (this->SpotShadowDescritors)
    {
        this->SpotShadowDescritors->ResetSceneState();
    }
}

void LightManager::CleanLastResources()
{
    _lights.clear();
    DirLights.clear();
    SpotLights.clear();
    PointLights.clear();

    lightBuffer.clear();
    sortedLight.clear();
    lights_bin.clear();
    lights_index.clear();
    light_tiles_bits.clear();

    currentNumLights = 0;

    lightUBO.reset();
    lightSSBO.reset();
    lightIndexSSBO.reset();
    lightTilesSSBO.reset();
    lightBinSSBO.reset();
    lightManagerUniform.reset();

    PointShadowDescritors.reset();
    CSMDescritors.reset();
    SpotShadowDescritors.reset();

    CSMPipelineModule.reset();
    OmniShadowPipelineModule.reset();
    CSMShaderModule.reset();
    OmniShadowShaderModule.reset();
}

void LightManager::CleanShadowMapResources()
{
    for (auto& pLight : this->PointLights)
    {
        if (pLight)
            pLight->CleanShadowMapResources();
    }

    for (auto& dLight : this->DirLights)
    {
        if (dLight)
            dLight->CleanShadowMapResources();
    }

    for (auto& sLight : this->SpotLights)
    {
        if (sLight)
            sLight->CleanShadowMapResources();
    }

    if (this->PointShadowDescritors)
    {
        this->PointShadowDescritors->Clean();
    }

    if (this->CSMDescritors)
    {
        this->CSMDescritors->Clean();
    }

    if (this->SpotShadowDescritors)
    {
        this->SpotShadowDescritors->Clean();
    }
}

void LightManager::AddLight(std::shared_ptr<QELight> light_ptr, std::string& name)
{
    this->lightBuffer.push_back(*light_ptr->uniform);

    if (this->_lights.find(name) == _lights.end())
    {
        this->_lights[name] = light_ptr;
    }
    else
    {
        name += "_1";
        this->_lights[name] = light_ptr;
    }

    light_ptr->Name = name;

    this->currentNumLights++;
}

void LightManager::SortingLights()
{
    auto activeCamera = QECameraContext::getInstance()->ActiveCamera();

    this->sortedLight.clear();
    this->sortedLight.reserve(this->lightBuffer.size());
    this->lights_index.clear();
    this->lights_index.reserve(this->lightBuffer.size());

    float near = activeCamera->GetNear();
    float far = activeCamera->GetFar();
    for (uint32_t i = 0; i < this->lightBuffer.size(); i++)
    {
        glm::vec4 position = glm::vec4(this->lightBuffer.at(i).position, 1.0f);
        glm::vec4 projected_position = activeCamera->CameraData->View * position;
        const float radius = this->lightBuffer.at(i).radius;
        const float centerDepth = -projected_position.z;
        const float minDepth = glm::max(near, centerDepth - radius);
        const float maxDepth = glm::min(far, centerDepth + radius);

        this->sortedLight.push_back({
                .id = i,
                .projected_z = ((centerDepth - near) / (far - near)),
                .projected_z_min = ((minDepth - near) / (far - near)),
                .projected_z_max = ((maxDepth - near) / (far - near))
        });
    }

    std::sort(this->sortedLight.begin(), this->sortedLight.end(), compareDistance);

    for (uint32_t i = 0; i < this->sortedLight.size(); i++)
    {
        lights_index.push_back(this->sortedLight.at(i).id);
    }
}

void LightManager::ComputeLightsLUT()
{
    this->lights_bin.clear();
    this->lights_bin.reserve(BIN_SLICES);

    float bin_size = 1.0f / BIN_SLICES;

    for (uint32_t bin = 0; bin < BIN_SLICES; bin++)
    {
        uint32_t min_light_id = static_cast<uint32_t>(this->sortedLight.size() + 1);
        uint32_t max_light_id = 0;

        float bin_min = bin_size * bin;
        float bin_max = bin_min + bin_size;

        for (uint32_t i = 0; i < this->sortedLight.size(); i++)
        {
            const LightMap& light = this->sortedLight.at(i);
            const LightUniform& lightUniform = this->lightBuffer.at(light.id);

            if (lightUniform.lightType == DIRECTIONAL_LIGHT || lightUniform.lightType == SUN_LIGHT)
            {
                if (i < min_light_id)
                {
                    min_light_id = i;
                }

                if (i > max_light_id)
                {
                    max_light_id = i;
                }

                continue;
            }

            bool isInside = light.projected_z >= bin_min && light.projected_z <= bin_max;
            bool isInsideMinor = light.projected_z_min <= bin_min && light.projected_z_min <= bin_max && light.projected_z >= bin_min;
            bool isInsideMayor = light.projected_z_max >= bin_min && light.projected_z_max >= bin_max && light.projected_z <= bin_max;
            bool isMinor = light.projected_z_min >= bin_min && light.projected_z_min <= bin_max;
            bool isMayor = light.projected_z_max <= bin_max && light.projected_z_max >= bin_min;

            if (isInside || isMinor || isMayor || isInsideMinor || isInsideMayor)
            {
                if (i < min_light_id)
                {
                    min_light_id = i;
                }

                if (i > max_light_id)
                {
                    max_light_id = i;
                }
            }
        }

        this->lights_bin.push_back(min_light_id | (max_light_id << 16));
    }
}

void LightManager::ComputeLightTiles()
{
    auto activeCamera = QECameraContext::getInstance()->ActiveCamera();
    if (!activeCamera)
        return;

    VkExtent2D renderExtent = swapChainModule->swapChainExtent;
    auto cameraContext = QECameraContext::getInstance();
    if (cameraContext && cameraContext->GetRenderTargetOverride() && cameraContext->GetRenderTargetOverride()->Valid())
    {
        renderExtent = cameraContext->GetRenderTargetOverride()->Extent;
    }

    uint32_t tileXCount = (renderExtent.width + swapChainModule->TILE_SIZE - 1) / swapChainModule->TILE_SIZE;
    uint32_t tileYCount = (renderExtent.height + swapChainModule->TILE_SIZE - 1) / swapChainModule->TILE_SIZE;

    // Calculamos el n�mero total de entradas de tiles
    uint32_t tilesEntryCount = tileXCount * tileYCount * NUM_WORDS;
    uint32_t newTileSize = swapChainModule->TILE_SIZE;

    // Si el n�mero total de entradas de tiles excede el m�ximo, ajustamos din�micamente el tama�o del tile
    while (tilesEntryCount > MAX_NUM_TILES)
    {
        // Aumentamos el tama�o del tile para reducir el n�mero total de tiles
        newTileSize += 1;

        // Recalculamos el n�mero de tiles con el nuevo tama�o de tile
        tileXCount = (renderExtent.width + newTileSize - 1) / newTileSize;
        tileYCount = (renderExtent.height + newTileSize - 1) / newTileSize;

        // Recalculamos el n�mero total de entradas de tiles
        tilesEntryCount = tileXCount * tileYCount * NUM_WORDS;
    }

    const uint32_t currentFrame = static_cast<uint32_t>(SynchronizationModule::GetCurrentFrame());
    this->swapChainModule->UpdateTileSize((float)newTileSize);
    this->swapChainModule->UpdateScreenData(renderExtent, currentFrame);

    const uint32_t tile_x_count = tileXCount;
    const uint32_t tile_y_count = tileYCount;
    const uint32_t tiles_entry_count = tile_x_count * tile_y_count * NUM_WORDS;

    this->light_tiles_bits.clear();
    this->light_tiles_bits.resize(tiles_entry_count, 0u);

    float near_z = activeCamera->GetNear();
    float tile_size_inv = 1.0f / newTileSize;

    uint32_t tile_stride = tile_x_count * NUM_WORDS;
    const uint32_t visibleLightCount = static_cast<uint32_t>(
        std::min(this->lights_index.size(), this->lightBuffer.size()));

    for (uint32_t i = 0; i < visibleLightCount; i++)
    {
        const uint32_t light_index = this->lights_index.at(i);
        if (light_index >= this->lightBuffer.size())
            continue;

        LightUniform& light = this->lightBuffer.at(light_index);

        if (light.lightType == DIRECTIONAL_LIGHT || light.lightType == SUN_LIGHT)
        {
            uint32_t word_index = i / 32;
            uint32_t bit_index = i % 32;
            uint32_t mask = (1u << bit_index);

            for (uint32_t tile_entry = word_index; tile_entry < tiles_entry_count; tile_entry += NUM_WORDS)
            {
                light_tiles_bits[tile_entry] |= mask;
            }

            continue;
        }

        // Fallback conservador: el culling por tiles de luces locales sigue siendo inestable
        // y produce bandas visibles en pantalla. Mientras lo reescribimos con una proyeccion
        // correcta, marcamos point/spot lights como visibles en todos los tiles para priorizar
        // correccion visual sobre optimizacion.
        {
            uint32_t word_index = i / 32;
            uint32_t bit_index = i % 32;
            uint32_t mask = (1u << bit_index);

            for (uint32_t tile_entry = word_index; tile_entry < tiles_entry_count; tile_entry += NUM_WORDS)
            {
                light_tiles_bits[tile_entry] |= mask;
            }

            continue;
        }

        glm::vec4 pos{ light.position.x, light.position.y, light.position.z, 1.0f };
        float radius = light.radius;

        glm::vec4 view_space_pos = activeCamera->CameraData->View * pos;
        const float sphereMinDepth = -view_space_pos.z - radius;
        const float sphereMaxDepth = -view_space_pos.z + radius;

        if (sphereMaxDepth < near_z)
        {
            continue;
        }

        glm::vec4 aabb(0.0f);

        glm::vec3 aabb_min{ FLT_MAX, FLT_MAX ,FLT_MAX }, aabb_max{ -FLT_MAX ,-FLT_MAX ,-FLT_MAX };

        for (uint32_t c = 0; c < 8; ++c) {
            glm::vec3 corner{ (c % 2) ? 1.f : -1.f, (c & 2) ? 1.f : -1.f, (c & 4) ? 1.f : -1.f };
            corner = corner * radius;
            corner = corner + glm::vec3(pos);

            glm::vec4 corner_vs = activeCamera->CameraData->View * glm::vec4(corner, 1.f);
            corner_vs.z = glm::min(corner_vs.z, -near_z);

            glm::vec4 corner_ndc = activeCamera->CameraData->Projection * corner_vs;
            corner_ndc = corner_ndc / corner_ndc.w;

            aabb_min.x = glm::min(aabb_min.x, corner_ndc.x);
            aabb_min.y = glm::min(aabb_min.y, corner_ndc.y);

            aabb_max.x = glm::max(aabb_max.x, corner_ndc.x);
            aabb_max.y = glm::max(aabb_max.y, corner_ndc.y);
        }

        aabb.x = aabb_min.x;
        aabb.z = aabb_max.x;
        aabb.w = -1 * aabb_min.y;
        aabb.y = -1 * aabb_max.y;

        glm::vec4 aabb_screen{ (aabb.x * 0.5f + 0.5f) * (renderExtent.width - 1),
                           (aabb.y * 0.5f + 0.5f) * (renderExtent.height - 1),
                           (aabb.z * 0.5f + 0.5f) * (renderExtent.width - 1),
                           (aabb.w * 0.5f + 0.5f) * (renderExtent.height - 1) };

        float width = aabb_screen.z - aabb_screen.x;
        float height = aabb_screen.w - aabb_screen.y;

        if (width < 0.0001f || height < 0.0001f) {
            continue;
        }

        float min_x = aabb_screen.x;
        float min_y = aabb_screen.y;

        float max_x = min_x + width;
        float max_y = min_y + height;

        if (min_x > renderExtent.width || min_y > renderExtent.height) {
            continue;
        }

        if (max_x < 0.0f || max_y < 0.0f) {
            continue;
        }

        min_x = glm::max(min_x, 0.0f);
        min_y = glm::max(min_y, 0.0f);

        max_x = glm::min(max_x, (float)renderExtent.width);
        max_y = glm::min(max_y, (float)renderExtent.height);

        uint32_t first_tile_x = (uint32_t)(min_x * tile_size_inv);
        uint32_t last_tile_x = glm::min(tile_x_count - 1, (uint32_t)(max_x * tile_size_inv));

        uint32_t first_tile_y = (uint32_t)(min_y * tile_size_inv);
        uint32_t last_tile_y = glm::min(tile_y_count - 1, (uint32_t)(max_y * tile_size_inv));

        for (uint32_t y = first_tile_y; y <= last_tile_y; ++y) {
            for (uint32_t x = first_tile_x; x <= last_tile_x; ++x) {
                uint32_t array_index = y * tile_stride + x * NUM_WORDS;

                uint32_t word_index = i / 32;
                uint32_t bit_index = i % 32;

                if (array_index + word_index < light_tiles_bits.size())
                {
                    light_tiles_bits[array_index + word_index] |= (1u << bit_index);
                }
            }
        }
    }
}

void LightManager::Update(uint32_t currentFrame)
{
    if (_lights.empty())
    {
        currentNumLights = 0;
        lightBuffer.clear();
        sortedLight.clear();
        lights_bin.assign(BIN_SLICES, 0);
        lights_index.clear();
        light_tiles_bits.clear();
        UpdateUBOLight();
        return;
    }

    this->SortingLights();
    this->currentNumLights = static_cast<uint32_t>(this->lights_index.size());
    this->ComputeLightsLUT();
    this->ComputeLightTiles();

    if (!this->lights_index.empty())
    {
        void* data2;
        uint32_t indexSize = static_cast<uint32_t>(this->lights_index.size()) * sizeof(uint32_t);

        vkMapMemory(this->deviceModule->device, this->lightIndexSSBO->uniformBuffersMemory[currentFrame], 0, indexSize, 0, &data2);
        memcpy(data2, this->lights_index.data(), indexSize);
        vkUnmapMemory(this->deviceModule->device, this->lightIndexSSBO->uniformBuffersMemory[currentFrame]);
    }

    if (!this->light_tiles_bits.empty())
    {
        void* data3;
        size_t tilesSize = this->light_tiles_bits.size() * sizeof(uint32_t);
        vkMapMemory(this->deviceModule->device, this->lightTilesSSBO->uniformBuffersMemory[currentFrame], 0, tilesSize, 0, &data3);
        memcpy(data3, this->light_tiles_bits.data(), tilesSize);
        vkUnmapMemory(this->deviceModule->device, this->lightTilesSSBO->uniformBuffersMemory[currentFrame]);
    }

    void* data4;
    vkMapMemory(this->deviceModule->device, this->lightBinSSBO->uniformBuffersMemory[currentFrame], 0, this->lightBinSSBOSize, 0, &data4);
    memcpy(data4, this->lights_bin.data(), this->lightBinSSBOSize);
    vkUnmapMemory(this->deviceModule->device, this->lightBinSSBO->uniformBuffersMemory[currentFrame]);

    this->UpdateCSMLights();
    this->UpdateUniform();

    if (this->CSMDescritors)
    {
        this->CSMDescritors->UpdateResources(currentFrame);
    }

    if (this->SpotShadowDescritors)
    {
        this->SpotShadowDescritors->UpdateResources(currentFrame);
    }
}

void LightManager::ReindexShadowMaps()
{
    for (uint32_t i = 0; i < DirLights.size(); ++i)
    {
        if (DirLights[i])
        {
            DirLights[i]->idxShadowMap = i;
        }
    }

    for (uint32_t i = 0; i < PointLights.size(); ++i)
    {
        if (PointLights[i])
        {
            PointLights[i]->idxShadowMap = i;
        }
    }

    for (uint32_t i = 0; i < SpotLights.size(); ++i)
    {
        if (SpotLights[i])
        {
            SpotLights[i]->idxShadowMap = i;
        }
    }
}

void LightManager::SyncDirectionalLightIndices()
{
    for (uint32_t i = 0; i < DirLights.size(); ++i)
    {
        if (DirLights[i])
        {
            DirLights[i]->idxShadowMap = i;
        }
    }
}
