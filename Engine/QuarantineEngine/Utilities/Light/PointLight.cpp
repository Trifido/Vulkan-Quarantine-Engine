#include "PointLight.h"
#include <SynchronizationModule.h>

PointLight::PointLight() : Light()
{
    this->lightType = LightType::POINT_LIGHT;
}

PointLight::PointLight(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass)
{
    auto size = sizeof(OmnniShadowUniform);
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
}
