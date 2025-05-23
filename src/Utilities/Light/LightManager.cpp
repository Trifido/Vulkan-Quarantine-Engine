#include "LightManager.h"
#include <algorithm>
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include <SynchronizationModule.h>
#include <SunLight.h>

bool compareDistance(const LightMap& a, const LightMap& b)
{
    //if (a.projected_z < b.projected_z) return -1;
    //if (a.projected_z > b.projected_z) return 1;
    return a.projected_z < b.projected_z;
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

void LightManager::CreateLight(LightType type, std::string name)
{
    switch (type)
    {
        default:
        case LightType::POINT_LIGHT:
            this->PointLights.push_back(std::make_shared<PointLight>(this->renderPassModule->OmniShadowMappingRenderPass));
            this->PointLights.back()->idxShadowMap = (uint32_t)this->PointLights.size() - 1;

            this->AddLight(std::static_pointer_cast<Light>(this->PointLights.back()), name);
            this->PointShadowDescritors->AddPointLightResources(
                this->PointLights.back()->shadowMappingResourcesPtr->shadowMapUBO,
                this->PointLights.back()->shadowMappingResourcesPtr->CubemapImageView,
                this->PointLights.back()->shadowMappingResourcesPtr->CubemapSampler);
            break;

        case LightType::SUN_LIGHT:
        case LightType::DIRECTIONAL_LIGHT:
            if (type == LightType::SUN_LIGHT)
            {
                this->DirLights.push_back(std::make_shared<SunLight>(this->renderPassModule->DirShadowMappingRenderPass, this->camera));
            }
            else
            {
                this->DirLights.push_back(std::make_shared<DirectionalLight>(this->renderPassModule->DirShadowMappingRenderPass, this->camera));
            }
            this->DirLights.back()->idxShadowMap = (uint32_t)this->DirLights.size() - 1;

            this->AddLight(std::static_pointer_cast<Light>(this->DirLights.back()), name);
            this->CSMDescritors->AddDirLightResources(
                this->DirLights.back()->shadowMappingResourcesPtr->OffscreenShadowMapUBO,
                this->DirLights.back()->shadowMappingResourcesPtr->CSMImageView,
                this->DirLights.back()->shadowMappingResourcesPtr->CSMSampler);

            this->CSMDescritors->BindResources(this->DirLights.back()->shadowMappingResourcesPtr->CascadeResourcesPtr);
            break;

        case LightType::SPOT_LIGHT:
            this->SpotLights.push_back(std::make_shared<SpotLight>(this->CSMShaderModule, this->renderPassModule->DirShadowMappingRenderPass));
            this->AddLight(std::static_pointer_cast<Light>(this->SpotLights.back()), name);
            break;
    }
}

void LightManager::LoadLightDtos(const std::vector<LightDto>& lightDtos)
{
    for (const auto& lightDto : lightDtos)
    {
        this->CreateLight(lightDto.lightType, lightDto.name);

        auto light = this->GetLight(lightDto.name);
        if (light)
        {
            light->transform->SetModel(lightDto.worldTransform);
            light->diffuse = lightDto.diffuse;
            light->specular = lightDto.specular;
            light->SetDistanceEffect(lightDto.radius);
            light->cutOff = lightDto.cutOff;
            light->outerCutoff = lightDto.outerCutoff;
        }
    }
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
        file.write(reinterpret_cast<const char*>(&light->transform->GetModel()), sizeof(glm::mat4));
        file.write(reinterpret_cast<const char*>(&light->diffuse), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&light->specular), sizeof(glm::vec3));
        file.write(reinterpret_cast<const char*>(&light->cutOff), sizeof(float));
        file.write(reinterpret_cast<const char*>(&light->outerCutoff), sizeof(float));
    }
}

std::shared_ptr<Light> LightManager::GetLight(std::string name)
{
    auto it =_lights.find(name);

    if (it != _lights.end())
        return it->second;

    return nullptr;
}

void LightManager::InitializeShadowMaps()
{
    this->PointShadowDescritors->InitializeDescriptorSetLayouts(this->OmniShadowShaderModule);
    this->CSMDescritors->InitializeDescriptorSetLayouts(this->CSMShaderModule);
}

void LightManager::UpdateUniform()
{
    this->lightManagerUniform->numLights = (uint32_t)std::min(this->_lights.size(), this->MAX_NUM_LIGHT);

    int cont = 0;
    for (auto& it : this->_lights)
    {
        it.second->UpdateUniform();

        this->lightBuffer[cont] = *it.second->uniform;
        cont++;
    }

    this->UpdateUBOLight();
}

void LightManager::UpdateUBOLight()
{
    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->lightUBO->uniformBuffersMemory[currentFrame], 0, sizeof(LightManagerUniform), 0, &data);
        memcpy(data, static_cast<const void*>(this->lightManagerUniform.get()), sizeof(LightManagerUniform));
        vkUnmapMemory(this->deviceModule->device, this->lightUBO->uniformBuffersMemory[currentFrame]);

        void* data2;
        vkMapMemory(this->deviceModule->device, this->lightSSBO->uniformBuffersMemory[currentFrame], 0, this->lightSSBOSize, 0, &data2);
        memcpy(data2, this->lightBuffer.data(), this->lightSSBOSize);
        vkUnmapMemory(this->deviceModule->device, this->lightSSBO->uniformBuffersMemory[currentFrame]);
    }
}

void LightManager::UpdateCSMLights()
{
    for (auto light : this->DirLights)
    {
        light->UpdateUniform();
    }
}

void LightManager::CleanLightUBO()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Light UBO
        if (this->lightManagerUniform != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->lightUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightUBO->uniformBuffersMemory[i], nullptr);

            vkDestroyBuffer(deviceModule->device, this->lightSSBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightSSBO->uniformBuffersMemory[i], nullptr);

            vkDestroyBuffer(deviceModule->device, this->lightIndexSSBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightIndexSSBO->uniformBuffersMemory[i], nullptr);

            vkDestroyBuffer(deviceModule->device, this->lightTilesSSBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightTilesSSBO->uniformBuffersMemory[i], nullptr);

            vkDestroyBuffer(deviceModule->device, this->lightBinSSBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->lightBinSSBO->uniformBuffersMemory[i], nullptr);
        }
    }
}

void LightManager::CleanLastResources()
{
    this->_lights.clear();
    this->lightUBO.reset();
    this->lightManagerUniform.reset();
}

void LightManager::CleanShadowMapResources()
{
    for (auto pLight : this->PointLights)
    {
        pLight->CleanShadowMapResources();
    }
    for (auto dLight : this->DirLights)
    {
        dLight->CleanShadowMapResources();
    }
    for (auto sLight : this->SpotLights)
    {

    }

    this->PointShadowDescritors->Clean();
    this->CSMDescritors->Clean();
}

void LightManager::SetCamera(Camera* camera_ptr)
{
    this->camera = camera_ptr;
}

void LightManager::AddLight(std::shared_ptr<Light> light_ptr, std::string& name)
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

    this->currentNumLights++;
}

void LightManager::SortingLights()
{
    this->sortedLight.clear();
    this->sortedLight.reserve(this->lightBuffer.size());
    this->lights_index.clear();
    this->lights_index.reserve(this->lightBuffer.size());

    float near = *this->camera->GetRawNearPlane();
    float far = *this->camera->GetRawFarPlane();
    for (uint32_t i = 0; i < this->lightBuffer.size(); i++)
    {
        glm::vec4 position = glm::vec4(this->lightBuffer.at(i).position, 1.0f);
        glm::vec4 p_min = position + glm::vec4(this->camera->cameraFront * -this->lightBuffer.at(i).radius, 0.0f);
        glm::vec4 p_max = position + glm::vec4(this->camera->cameraFront * this->lightBuffer.at(i).radius, 0.0f);

        glm::vec4 projected_position = this->camera->view * position;
        glm::vec4 projected_p_min = this->camera->view * p_min;
        glm::vec4 projected_p_max = this->camera->view * p_max;

        this->sortedLight.push_back({
                .id = i,
                .projected_z = ((-projected_position.z - near) / (far - near)),
                .projected_z_min = ((-projected_p_min.z - near) / (far - near)),
                .projected_z_max = ((-projected_p_max.z - near) / (far - near))
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
    uint32_t tileXCount = swapChainModule->swapChainExtent.width / swapChainModule->TILE_SIZE;
    uint32_t tileYCount = swapChainModule->swapChainExtent.height / swapChainModule->TILE_SIZE;

    // Calculamos el n�mero total de entradas de tiles
    uint32_t tilesEntryCount = tileXCount * tileYCount * NUM_WORDS;
    uint32_t newTileSize = swapChainModule->TILE_SIZE;

    // Si el n�mero total de entradas de tiles excede el m�ximo, ajustamos din�micamente el tama�o del tile
    while (tilesEntryCount > MAX_NUM_TILES)
    {
        // Aumentamos el tama�o del tile para reducir el n�mero total de tiles
        newTileSize += 1;

        // Recalculamos el n�mero de tiles con el nuevo tama�o de tile
        tileXCount = swapChainModule->swapChainExtent.width / newTileSize;
        tileYCount = swapChainModule->swapChainExtent.height / newTileSize;

        // Recalculamos el n�mero total de entradas de tiles
        tilesEntryCount = tileXCount * tileYCount * NUM_WORDS;
    }

    this->swapChainModule->UpdateTileSize((float)newTileSize);

    const uint32_t tile_x_count = tileXCount;
    const uint32_t tile_y_count = tileYCount;
    const uint32_t tiles_entry_count = tile_x_count * tile_y_count * NUM_WORDS;

    this->light_tiles_bits.clear();
    this->light_tiles_bits.resize(tiles_entry_count, 0u);

    float near_z = *this->camera->GetRawNearPlane();
    float tile_size_inv = 1.0f / newTileSize;

    uint32_t tile_stride = tile_x_count * NUM_WORDS;

    for (uint32_t i = 0; i < this->currentNumLights; i++)
    {
        const uint32_t light_index = this->lights_index[i];
        LightUniform& light = this->lightBuffer.at(light_index);

        glm::vec4 pos{ light.position.x, light.position.y, light.position.z, 1.0f };
        float radius = light.radius;

        glm::vec4 view_space_pos = camera->view * pos;
        glm::vec2 cx{ view_space_pos.x, view_space_pos.z };
        const float tx_squared = glm::dot(cx, cx) - (radius * radius);
        glm::vec2 vx{ sqrtf(tx_squared), radius };
        glm::mat2 xtransf_min{ vx.x, vx.y, -vx.y, vx.x };
        glm::vec2 minx = xtransf_min * cx;
        glm::mat2 xtransf_max{ vx.x, -vx.y, vx.y, vx.x };
        glm::vec2 maxx = xtransf_max * cx;

        glm::vec2 cy{ -view_space_pos.y, view_space_pos.z };
        const float ty_squared = glm::dot(cy, cy) - (radius * radius);
        glm::vec2 vy{ sqrtf(ty_squared), radius };
        glm::mat2 ytransf_min{ vy.x, vy.y, -vy.y, vy.x };
        glm::vec2 miny = ytransf_min * cy;
        glm::mat2 ytransf_max{ vy.x, -vy.y, vy.y, vy.x };
        glm::vec2 maxy = ytransf_max * cy;

        glm::vec4 aabb{ minx.x / minx.y * this->camera->projection[0][0], miny.x / miny.y * this->camera->projection[1][1],
                    maxx.x / maxx.y * this->camera->projection[0][0], maxy.x / maxy.y * this->camera->projection[1][1] };


        // Build view space AABB and project it, then calculate screen AABB
        glm::vec3 aabb_min{ FLT_MAX, FLT_MAX ,FLT_MAX }, aabb_max{ -FLT_MAX ,-FLT_MAX ,-FLT_MAX };

        for (uint32_t c = 0; c < 8; ++c) {
            glm::vec3 corner{ (c % 2) ? 1.f : -1.f, (c & 2) ? 1.f : -1.f, (c & 4) ? 1.f : -1.f };
            corner = corner * radius;
            corner = corner + glm::vec3(pos);

            // transform in view space
            glm::vec4 corner_vs = this->camera->view * glm::vec4(corner, 1.f);
            // adjust z on the near plane.
            // visible Z is negative, thus corner vs will be always negative, but near is positive.
            // get positive Z and invert ad the end.
            corner_vs.z = glm::max(near_z, corner_vs.z);

            glm::vec4 corner_ndc = this->camera->projection * corner_vs;
            corner_ndc = corner_ndc / corner_ndc.w;

            // clamp
            aabb_min.x = glm::min(aabb_min.x, corner_ndc.x);
            aabb_min.y = glm::min(aabb_min.y, corner_ndc.y);

            aabb_max.x = glm::max(aabb_max.x, corner_ndc.x);
            aabb_max.y = glm::max(aabb_max.y, corner_ndc.y);
        }

        aabb.x = aabb_min.x;
        aabb.z = aabb_max.x;
        // Inverted Y aabb
        aabb.w = -1 * aabb_min.y;
        aabb.y = -1 * aabb_max.y;

        glm::vec4 aabb_screen{ (aabb.x * 0.5f + 0.5f) * (swapChainModule->swapChainExtent.width - 1),
                           (aabb.y * 0.5f + 0.5f) * (swapChainModule->swapChainExtent.height - 1),
                           (aabb.z * 0.5f + 0.5f) * (swapChainModule->swapChainExtent.width - 1),
                           (aabb.w * 0.5f + 0.5f) * (swapChainModule->swapChainExtent.height - 1) };

        float width = aabb_screen.z - aabb_screen.x;
        float height = aabb_screen.w - aabb_screen.y;

        if (width < 0.0001f || height < 0.0001f) {
            continue;
        }

        float min_x = aabb_screen.x;
        float min_y = aabb_screen.y;

        float max_x = min_x + width;
        float max_y = min_y + height;

        if (min_x > swapChainModule->swapChainExtent.width || min_y > swapChainModule->swapChainExtent.height) {
            continue;
        }

        if (max_x < 0.0f || max_y < 0.0f) {
            continue;
        }

        min_x = glm::max(min_x, 0.0f);
        min_y = glm::max(min_y, 0.0f);

        max_x = glm::min(max_x, (float)swapChainModule->swapChainExtent.width);
        max_y = glm::min(max_y, (float)swapChainModule->swapChainExtent.height);

        uint32_t first_tile_x = (uint32_t)(min_x * tile_size_inv);
        uint32_t last_tile_x = glm::min(tile_x_count - 1, (uint32_t)(max_x * tile_size_inv));

        uint32_t first_tile_y = (uint32_t)(min_y * tile_size_inv);
        uint32_t last_tile_y = glm::min(tile_y_count - 1, (uint32_t)(max_y * tile_size_inv));

        for (uint32_t y = first_tile_y; y <= last_tile_y; ++y) {
            for (uint32_t x = first_tile_x; x <= last_tile_x; ++x) {
                uint32_t array_index = y * tile_stride + x;

                uint32_t word_index = i / 32;
                uint32_t bit_index = i % 32;

                light_tiles_bits[array_index + word_index] |= (1 << bit_index);
            }
        }
    }
}

void LightManager::Update()
{
    this->SortingLights();
    this->ComputeLightsLUT();
    this->ComputeLightTiles();

    uint32_t currentFrame = static_cast<uint32_t>(SynchronizationModule::GetCurrentFrame());

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

    this->CSMDescritors->UpdateResources(currentFrame);
}
