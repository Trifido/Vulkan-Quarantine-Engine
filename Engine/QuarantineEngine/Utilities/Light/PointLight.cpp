#include <PointLight.h>
#include <numbers>

PointLight::PointLight() : Light()
{
    this->lightType = LightType::POINT_LIGHT;
}

PointLight::PointLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass)
{
    this->shadowMappingResourcesPtr = std::make_shared<OmniShadowResources>(renderPass);
}

void PointLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->radius = this->radius;

    OmniShadowUniform omniParameters = {};
    omniParameters.lightPos = glm::vec4(this->transform->Position, 1.0f);
    omniParameters.model = glm::translate(glm::mat4(1.0f), glm::vec3(-omniParameters.lightPos.x, -omniParameters.lightPos.y, -omniParameters.lightPos.z));
    omniParameters.projection = glm::perspective((float)(std::numbers::pi / 2.0f), 1.0f, 0.01f, this->radius);

    this->shadowMappingResourcesPtr->UpdateUBOShadowMap(omniParameters);
}

void PointLight::CleanShadowMapResources()
{
    this->shadowMappingResourcesPtr->Cleanup();
}
