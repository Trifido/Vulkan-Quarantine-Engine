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

    this->baseIntensity = 100.0f;
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

void SunLight::SetLightDirection(glm::vec3 dir)
{
    this->transform->ForwardVector = glm::normalize(dir);
    this->uniformData.Direction = this->transform->ForwardVector;

    float elevation = -glm::clamp(this->transform->ForwardVector.y, -1.0f, 1.0f);
    float colorIntensity = glm::clamp(elevation, 0.0f, 1.0f);
    this->uniformData.Intensity = this->baseIntensity * colorIntensity;

    glm::vec3 sunsetColor = glm::vec3(1.0f, 0.4f, 0.2f); 
    glm::vec3 dayColor = glm::vec3(1.0f, 1.0f, 0.9f);

    this->diffuse = glm::mix(sunsetColor, dayColor, colorIntensity) * colorIntensity;
    this->specular = glm::vec3(0.1f) * this->diffuse * colorIntensity;
}
