#include "DirectionalLight.h"
#include <SynchronizationModule.h>

DirectionalLight::DirectionalLight() : Light()
{
    this->lightType = LightType::DIRECTIONAL_LIGHT;
    this->radius = FLT_MAX;

    this->transform->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    this->transform->SetOrientation(glm::vec3(90.0f, 0.0f, 0.0f));
}

DirectionalLight::DirectionalLight(std::shared_ptr<VkRenderPass> renderPass, Camera* camera) : DirectionalLight()
{
    this->camera = camera;
    auto size = sizeof(glm::mat4);
    this->shadowMappingResourcesPtr = std::make_shared<CSMResources>(renderPass);
}

void DirectionalLight::UpdateUniform()
{
    Light::UpdateUniform();

    this->uniform->position = this->transform->Position;
    this->uniform->direction = glm::normalize(this->transform->Rotation);

    glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f);

    glm::mat4 lightview = glm::lookAt(this->transform->Position, this->transform->ForwardVector, this->transform->UpVector);
    glm::mat4 lightProjection = clip * glm::ortho(-10.f, 10.f, -10.f, 10.f, 0.01f, this->radius);
    glm::mat4 viewproj = lightProjection * lightview;

    //this->shadowMappingResourcesPtr->UpdateUBOShadowMap(omniParameters);
}

void DirectionalLight::UpdateCascades()
{
    float cascadeSplits[CSMResources::SHADOW_MAP_CASCADE_COUNT];

    float nearClip = camera->GetNear();
    float farClip = camera->GetFar();
    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < CSMResources::SHADOW_MAP_CASCADE_COUNT; i++) {
        float p = (i + 1) / static_cast<float>(CSMResources::SHADOW_MAP_CASCADE_COUNT);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = this->cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }

    // Calculate orthographic projection matrix for each cascade
    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < CSMResources::SHADOW_MAP_CASCADE_COUNT; i++) {
        float splitDist = cascadeSplits[i];

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f,  1.0f, 0.0f),
            glm::vec3(1.0f,  1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f,  1.0f,  1.0f),
            glm::vec3(1.0f,  1.0f,  1.0f),
            glm::vec3(1.0f, -1.0f,  1.0f),
            glm::vec3(-1.0f, -1.0f,  1.0f),
        };

        // Project frustum corners into world space
        glm::mat4 invCam = glm::inverse(camera->VP);
        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = invCorner / invCorner.w;
        }

        for (uint32_t j = 0; j < 4; j++) {
            glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
            frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
            frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t j = 0; j < 8; j++) {
            frustumCenter += frustumCorners[j];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (uint32_t j = 0; j < 8; j++) {
            float distance = glm::length(frustumCorners[j] - frustumCenter);
            radius = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

        glm::vec3 lightDir = glm::normalize(this->transform->Rotation);
        glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        // Store split distance and matrix in cascade
        this->shadowMappingResourcesPtr->cascadeResources[i].splitDepth = (nearClip + splitDist * clipRange) * -1.0f;
        this->shadowMappingResourcesPtr->cascadeResources[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }
}

void DirectionalLight::CleanShadowMapResources()
{
    this->shadowMappingResourcesPtr->Cleanup();
}
