#include "SunLight.h"
#include <SynchronizationModule.h>

QESunLight::QESunLight() : QEDirectionalLight()
{
    this->lightType = LightType::SUN_LIGHT;
    this->sunUBO = std::make_shared<UniformBufferObject>();
    this->sunUBO->CreateUniformBuffer(sizeof(SunUniform), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->baseIntensity = 100.0f;
    this->diffuse = glm::vec3(0.6f);
    this->specular = glm::vec3(0.1f);
    this->SetDistanceEffect(100.0f);
}

void QESunLight::UpdateSun()
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

void QESunLight::SetLightDirection(glm::vec3 dir)
{
    this->transform->SetLocalEulerDegrees(dir);
    glm::vec3 directionToSun = this->transform->Forward();
    this->uniformData.Direction = directionToSun;

    float elevation = glm::clamp(-directionToSun.y, -1.0f, 1.0f);
    float colorIntensity = glm::clamp(elevation, 0.0f, 1.0f);
    this->uniformData.Intensity = this->baseIntensity * colorIntensity;

    glm::vec3 sunsetColor = glm::vec3(1.0f, 0.4f, 0.2f); 
    glm::vec3 dayColor = glm::vec3(1.0f, 1.0f, 0.9f);

    this->diffuse = glm::mix(sunsetColor, dayColor, colorIntensity) * colorIntensity;
    this->specular = glm::vec3(0.1f) * this->diffuse * colorIntensity;
}
