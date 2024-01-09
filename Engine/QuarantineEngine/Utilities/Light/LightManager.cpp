#include "LightManager.h"
#include <algorithm>
#include "PointLight.h"
#include "DirectionalLight.h"
#include "SpotLight.h"
#include <SynchronizationModule.h>

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


void LightManager::SortLightByDepth()
{
}
