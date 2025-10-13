#include <PointLight.h>
#include <numbers>

QEPointLight::QEPointLight() : QELight()
{
    this->lightType = LightType::POINT_LIGHT;
}

void QEPointLight::Setup(std::shared_ptr<VkRenderPass> renderPass)
{
    this->shadowMappingResourcesPtr = std::make_shared<OmniShadowResources>(renderPass);
}

void QEPointLight::UpdateUniform()
{
    QELight::UpdateUniform();

    this->uniform->position = this->transform->GetWorldPosition();
    this->uniform->radius = this->radius;
    this->uniform->idxShadowMap = this->idxShadowMap;

    OmniShadowUniform omniParameters = {};
    omniParameters.lightPos = glm::vec4(this->uniform->position, 1.0f);
    omniParameters.projection = glm::perspective((float)(std::numbers::pi * 0.5), 1.0f, 0.01f, this->radius);

    this->shadowMappingResourcesPtr->UpdateUBOShadowMap(omniParameters);
}

void QEPointLight::CleanShadowMapResources()
{
    this->shadowMappingResourcesPtr->Cleanup();
}
