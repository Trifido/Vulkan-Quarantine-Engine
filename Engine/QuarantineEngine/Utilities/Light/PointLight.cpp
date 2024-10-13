#include "PointLight.h"
#include <SynchronizationModule.h>

PointLight::PointLight() : Light()
{
    this->lightType = LightType::POINT_LIGHT;
}

PointLight::PointLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass)
{
    auto size = sizeof(OmniShadowUniform);
    this->shadowMappingPtr = std::make_shared<ShadowMappingModule>(shaderModule, renderPass, ShadowMappingMode::OMNI_SHADOW);

    this->shadowMapUBO = std::make_shared<UniformBufferObject>();
    this->shadowMapUBO->CreateUniformBuffer(size, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    this->descriptorBuffer = std::make_shared<DescriptorBuffer>(shaderModule);
    this->descriptorBuffer->InitializeShadowMapDescritorSets(shaderModule, shadowMapUBO, size);
}

void PointLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->radius = this->radius;

    OmniShadowUniform omniParameters = {};
    omniParameters.lightPos = glm::vec4(this->transform->Position, 1.0f);
    omniParameters.model = glm::translate(glm::mat4(1.0f), glm::vec3(-omniParameters.lightPos.x, -omniParameters.lightPos.y, -omniParameters.lightPos.z));
    omniParameters.projection = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.01f, this->radius);

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame], 0, sizeof(OmniShadowUniform), 0, &data);
        memcpy(data, &omniParameters, sizeof(OmniShadowUniform));
        vkUnmapMemory(this->deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[currentFrame]);
    }
}

void PointLight::CleanShadowMapResources()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (this->shadowMapUBO != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->shadowMapUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->shadowMapUBO->uniformBuffersMemory[i], nullptr);
        }
    }

    this->shadowMappingPtr->cleanup();
    this->descriptorBuffer->Cleanup();
    this->descriptorBuffer->CleanLastResources();
}
