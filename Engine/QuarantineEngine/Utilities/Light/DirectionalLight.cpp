#include "DirectionalLight.h"
#include <SynchronizationModule.h>

DirectionalLight::DirectionalLight() : Light()
{
    this->lightType = LightType::DIRECTIONAL_LIGHT;
    this->radius = FLT_MAX;
}

DirectionalLight::DirectionalLight(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass, DeviceModule* deviceModule) : DirectionalLight()
{
    this->shadowMappingPtr = std::make_shared<ShadowMappingModule>(shaderModule, renderPass);

    this->shadowMapUBO = std::make_shared<UniformBufferObject>();
    this->shadowMapUBO->CreateUniformBuffer(sizeof(glm::mat4), MAX_FRAMES_IN_FLIGHT, *deviceModule);

    glm::mat4 clip = glm::mat4( 1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, -1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 0.5f, 0.0f,
                                0.0f, 0.0f, 0.5f, 1.0f);

    glm::mat4 lightview = glm::lookAt(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 lightProjection = clip * glm::ortho(-10.f, 10.f, -10.f, 10.f, 0.01f, 20.0f);
    glm::mat4 viewproj = lightProjection * lightview;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(glm::mat4), 0, &data);
        memcpy(data, &viewproj, sizeof(glm::mat4));
        vkUnmapMemory(deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame]);
    }

    this->descriptorBuffer = std::make_shared<DescriptorBuffer>(shaderModule);
    this->descriptorBuffer->InitializeShadowMapDescritorSets(shaderModule, shadowMapUBO);
}

void DirectionalLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Rotation;
}
