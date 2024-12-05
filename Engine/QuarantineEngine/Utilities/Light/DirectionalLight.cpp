#include "DirectionalLight.h"
#include <SynchronizationModule.h>

DirectionalLight::DirectionalLight() : Light()
{
    this->lightType = LightType::DIRECTIONAL_LIGHT;
    this->radius = FLT_MAX;

    this->transform->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    this->transform->SetOrientation(glm::vec3(90.0f, 0.0f, 0.0f));
}

DirectionalLight::DirectionalLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass) : DirectionalLight()
{
    auto size = sizeof(glm::mat4);
    this->shadowMappingResourcesPtr = std::make_shared<CSMResources>(shaderModule, renderPass, ShadowMappingMode::DIRECTIONAL_SHADOW);
}

void DirectionalLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->direction = this->transform->Rotation;

    glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f);

    glm::mat4 lightview = glm::lookAt(this->transform->Position, this->transform->ForwardVector, this->transform->UpVector);
    glm::mat4 lightProjection = clip * glm::ortho(-10.f, 10.f, -10.f, 10.f, 0.01f, this->radius);
    glm::mat4 viewproj = lightProjection * lightview;

    this->shadowMappingResourcesPtr->UpdateUBOShadowMap(omniParameters);
}

void DirectionalLight::CleanShadowMapResources()
{
    this->shadowMappingResourcesPtr->Cleanup();
}
