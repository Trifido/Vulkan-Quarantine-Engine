#include "SpotLight.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

QESpotLight::QESpotLight() : QELight()
{
    this->lightType = LightType::SPOT_LIGHT;

    this->cutOff = glm::cos(glm::radians(12.5f));
    this->outerCutoff = glm::cos(glm::radians(17.5f));
}

void QESpotLight::Setup(std::shared_ptr<VkRenderPass> renderPass)
{
    this->shadowMappingResourcesPtr = std::make_shared<SpotShadowResources>(renderPass);
}

void QESpotLight::UpdateUniform()
{
    QELight::UpdateUniform();

    if (!ResolveTransformFromOwner())
        return;

    this->uniform->cutOff = this->cutOff;
    this->uniform->outerCutoff = this->outerCutoff;
    this->uniform->position = this->transform->GetWorldPosition();
    this->uniform->direction = glm::normalize(this->transform->Forward());
    this->uniform->radius = this->radius;
    this->uniform->idxShadowMap = this->idxShadowMap;

    if (!this->shadowMappingResourcesPtr)
        return;

    float clampedOuterCutoff = glm::clamp(this->outerCutoff, -1.0f, 1.0f);
    float fovRadians = 2.0f * acos(clampedOuterCutoff);
    fovRadians = glm::clamp(fovRadians, glm::radians(1.0f), glm::radians(179.0f));

    glm::vec3 lightPosition = this->uniform->position;
    glm::vec3 lightDirection = this->uniform->direction;
    glm::vec3 up = this->transform->Up();

    if (glm::abs(glm::dot(lightDirection, glm::normalize(up))) > 0.99f)
    {
        up = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::mat4 lightView = glm::lookAt(lightPosition, lightPosition + lightDirection, up);
    glm::mat4 lightProjection = glm::perspective(fovRadians, 1.0f, 0.01f, this->radius);
    lightProjection[1][1] *= -1.0f;

    this->shadowMappingResourcesPtr->ViewProjMatrix = lightProjection * lightView;
    this->shadowMappingResourcesPtr->UpdateOffscreenUBOShadowMap();
}

void QESpotLight::CleanShadowMapResources()
{
    if (!this->shadowMappingResourcesPtr)
        return;

    this->shadowMappingResourcesPtr->Cleanup();
}
