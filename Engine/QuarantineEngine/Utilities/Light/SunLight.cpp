#include "SunLight.h"
#include <SynchronizationModule.h>

SunLight::SunLight() : DirectionalLight()
{
}

SunLight::SunLight(std::shared_ptr<VkRenderPass> renderPass, Camera* camera) :
    DirectionalLight(renderPass, camera)
{
    this->sunUBO = std::make_shared<UniformBufferObject>();
    this->sunUBO->CreateUniformBuffer(sizeof(SunUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->diffuse = glm::vec3(0.6f);
    this->specular = glm::vec3(0.1f);
    this->SetDistanceEffect(100.0f);
}

void SunLight::UpdateSun()
{
    this->UpdateUniform();

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame], 0, sizeof(SunUniform), 0, &data);
        memcpy(data, &uniformData, sizeof(SunUniform));
        vkUnmapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame]);
    }
}

void SunLight::SetParameters(glm::vec3 dir, float intensity)
{
    this->transform->ForwardVector = dir;
    uniformData.Direction = dir;
    uniformData.Intensity = intensity;
}
