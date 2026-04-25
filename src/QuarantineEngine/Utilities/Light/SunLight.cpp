#include "SunLight.h"
#include <SynchronizationModule.h>
#include <Helpers/QEMemoryTrack.h>

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
    EnsureRuntimeState();

    if (!this->sunUBO)
        return;

    this->UpdateUniform();

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data = nullptr;
        vkMapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame], 0, sizeof(SunUniform), 0, &data);
        memcpy(data, &uniformData, sizeof(SunUniform));
        vkUnmapMemory(deviceModule->device, this->sunUBO->uniformBuffersMemory[currentFrame]);
    }
}

void QESunLight::CleanupSunResources()
{
    this->CleanShadowMapResources();

    if (!this->sunUBO)
        return;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->sunUBO->uniformBuffers[i] != VK_NULL_HANDLE)
        {
            QE_DESTROY_BUFFER(deviceModule->device, this->sunUBO->uniformBuffers[i], "QESunLight::CleanupSunResources");
            this->sunUBO->uniformBuffers[i] = VK_NULL_HANDLE;
        }

        if (this->sunUBO->uniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            QE_FREE_MEMORY(deviceModule->device, this->sunUBO->uniformBuffersMemory[i], "QESunLight::CleanupSunResources");
            this->sunUBO->uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }

    this->sunUBO.reset();
    this->sunUBO = nullptr;
    this->ResourcesInitialized = false;
}

void QESunLight::SetSunEulerDegrees(const glm::vec3& eulerDeg)
{
    EnsureRuntimeState();

    this->SunEulerDegrees = eulerDeg;

    this->transform->SetLocalEulerDegrees(eulerDeg);

    glm::vec3 lightDirection = this->transform->Forward();
    this->uniformData.Direction = lightDirection;

    float elevation = glm::clamp(-lightDirection.y, -1.0f, 1.0f);
    float colorIntensity = glm::clamp(elevation, 0.0f, 1.0f);
    this->uniformData.Intensity = this->baseIntensity * colorIntensity;

    glm::vec3 sunsetColor = glm::vec3(1.0f, 0.4f, 0.2f);
    glm::vec3 dayColor = glm::vec3(1.0f, 1.0f, 0.9f);

    this->diffuse = glm::mix(sunsetColor, dayColor, colorIntensity) * colorIntensity;
    this->specular = glm::vec3(0.1f) * this->diffuse * colorIntensity;
}
