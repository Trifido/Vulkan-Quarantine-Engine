#include "SpotLight.h"
#include <SynchronizationModule.h>

SpotLight::SpotLight() : Light()
{
    this->lightType = LightType::SPOT_LIGHT;

    this->cutOff = glm::cos(glm::radians(12.5f));
    this->outerCutoff = glm::cos(glm::radians(17.5f));

    this->transform->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    this->transform->SetOrientation(glm::vec3(90.0f, 0.0f, 0.0f));
}

SpotLight::SpotLight(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass)
{
    auto size = sizeof(glm::mat4);
    this->shadowMappingPtr = std::make_shared<ShadowMappingModule>(shaderModule, renderPass);

    this->shadowMapUBO = std::make_shared<UniformBufferObject>();
    this->shadowMapUBO->CreateUniformBuffer(size, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->descriptorBuffer = std::make_shared<DescriptorBuffer>(shaderModule);
    this->descriptorBuffer->InitializeShadowMapDescritorSets(shaderModule, shadowMapUBO, size);
}

void SpotLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->cutOff = this->cutOff;
    this->uniform->outerCutoff = this->outerCutoff;
    this->uniform->position = this->transform->Position;
    this->uniform->direction = this->transform->Rotation;

    float fov = glm::degrees(acos(this->outerCutoff));

    glm::mat4 lightview = glm::lookAt(this->transform->Position, this->transform->ForwardVector, this->transform->UpVector);
    glm::mat4 lightProjection = glm::perspective(fov, 1.0f, 0.01f, this->radius);
    glm::mat4 viewproj = lightProjection * lightview;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(glm::mat4), 0, &data);
        memcpy(data, &viewproj, sizeof(glm::mat4));
        vkUnmapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}
