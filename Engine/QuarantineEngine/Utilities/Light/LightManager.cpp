#include "LightManager.h"
#include <algorithm>
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include <SynchronizationModule.h>

bool compareDistance(const LightMap& a, const LightMap& b)
{
    if (a.projected_z < b.projected_z) return -1;
    if (a.projected_z > b.projected_z) return 1;
    return 0;
}

LightManager* LightManager::instance = nullptr;

LightManager::LightManager()
{
    this->deviceModule = DeviceModule::getInstance();
    this->lightManagerUniform = std::make_shared<LightManagerUniform>();
    this->lightUBO = std::make_shared<UniformBufferObject>();
    this->lightUBO->CreateUniformBuffer(sizeof(LightManagerUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightSSBOSize = sizeof(LightUniform) * this->MAX_NUM_LIGHT;
    this->lightSSBO = std::make_shared<UniformBufferObject>();
    this->lightSSBO->CreateSSBO(this->lightSSBOSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->lightBuffer.reserve(this->MAX_NUM_LIGHT);
}

LightManager* LightManager::getInstance()
{
    if (instance == NULL)
        instance = new LightManager();

    return instance;
}

void LightManager::ResetInstance()
{
	delete instance;
	instance = nullptr;
}

void LightManager::CreateLight(LightType type, std::string name)
{
    switch (type)
    {
    default:
    case LightType::POINT_LIGHT:
        this->AddLight(std::static_pointer_cast<Light>(std::make_shared<PointLight>()), name);
        break;

    case LightType::DIRECTIONAL_LIGHT:
        this->AddLight(std::static_pointer_cast<Light>(std::make_shared<DirectionalLight>()), name);
        break;

    case LightType::SPOT_LIGHT:
        this->AddLight(std::static_pointer_cast<Light>(std::make_shared<SpotLight>()), name);
        break;
    }
}

std::shared_ptr<Light> LightManager::GetLight(std::string name)
{
    auto it =_lights.find(name);

    if (it != _lights.end())
        return it->second;

    return nullptr;
}

void LightManager::UpdateUniform()
{
    this->lightManagerUniform->numLights = std::min(this->_lights.size(), this->MAX_NUM_LIGHT);

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
        }
    }
}

void LightManager::CleanLastResources()
{
    this->_lights.clear();
    this->lightUBO.reset();
    this->lightManagerUniform.reset();
}

void LightManager::SetCamera(Camera* camera_ptr)
{
    this->camera = camera_ptr;
}

void LightManager::AddLight(std::shared_ptr<Light> light_ptr, std::string name)
{
    this->lightBuffer.push_back(*light_ptr->uniform);

    if (this->_lights.find(name) == _lights.end())
    {
        this->_lights[name] = light_ptr;
    }
    else
    {
        this->_lights[name + "_1"] = light_ptr;
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
        glm::vec4 projected_position = this->camera->view * position;

        glm::vec4 p_min = projected_position - glm::vec4(0.0f, 0.0f, this->lightBuffer.at(i).radius, 0.0f);
        glm::vec4 p_max = projected_position + glm::vec4(0.0f, 0.0f, this->lightBuffer.at(i).radius, 0.0f);

        this->sortedLight.push_back({
                .id = i,
                .projected_z = ((projected_position.z - near) / (far - near)),
                .projected_z_min = ((p_min.z - near) / (far - near)),
                .projected_z_max = ((p_max.z - near) / (far - near))
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
    const uint32_t BIN_SLICES = 16;

    this->lights_lut.clear();
    this->lights_lut.reserve(BIN_SLICES);

    float bin_size = 1.0f / BIN_SLICES;

    for (uint32_t bin = 0; bin < BIN_SLICES; bin++)
    {
        uint32_t min_light_id = this->sortedLight.size() + 1;
        uint32_t max_light_id = 0;

        float bin_min = bin_size * bin;
        float bin_max = bin_min + bin_size;

        for (uint32_t i = 0; i < this->sortedLight.size(); i++)
        {
            const LightMap& light = this->sortedLight.at(i);

            if ((light.projected_z >= bin_min && light.projected_z <= bin_max) ||
                (light.projected_z_min >= bin_min && light.projected_z_min <= bin_max) ||
                (light.projected_z_max >= bin_min && light.projected_z_max <= bin_max))
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

        this->lights_lut.push_back(min_light_id | (max_light_id << 16));
    }
}
