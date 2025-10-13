#include "SpotLight.h"
#include <SynchronizationModule.h>

QESpotLight::QESpotLight() : QELight()
{
    this->lightType = LightType::SPOT_LIGHT;

    this->cutOff = glm::cos(glm::radians(12.5f));
    this->outerCutoff = glm::cos(glm::radians(17.5f));

    this->transform->SetLocalPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    this->transform->SetLocalEulerDegrees(glm::vec3(90.0f, 0.0f, 0.0f));
}

QESpotLight::QESpotLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass) : QESpotLight()
{
    auto size = sizeof(glm::mat4);
    //this->shadowMappingPtr = std::make_shared<OmniShadowResources>(shaderModule, renderPass, ShadowMappingMode::DIRECTIONAL_SHADOW);

    this->shadowMapUBO = std::make_shared<UniformBufferObject>();
    this->shadowMapUBO->CreateUniformBuffer(size, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    //this->descriptorBuffer = std::make_shared<DescriptorBuffer>(shaderModule);
    //this->descriptorBuffer->InitializeShadowMapDescritorSets(shaderModule, shadowMapUBO, size);
}

void QESpotLight::UpdateUniform()
{
    QELight::UpdateUniform();

    this->uniform->cutOff = this->cutOff;
    this->uniform->outerCutoff = this->outerCutoff;
    this->uniform->position = this->transform->GetWorldPosition();
    this->uniform->direction = glm::normalize(this->transform->Forward());

    float fovRadians = acos(glm::clamp(this->outerCutoff, -1.0f, 1.0f)); // en radianes

    glm::mat4 lightview = glm::lookAt(this->uniform->position, this->uniform->direction, this->transform->Up());
    glm::mat4 lightProjection = glm::perspective(fovRadians, 1.0f, 0.01f, this->radius);
    glm::mat4 viewproj = lightProjection * lightview;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(glm::mat4), 0, &data);
        memcpy(data, &viewproj, sizeof(glm::mat4));
        vkUnmapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}
