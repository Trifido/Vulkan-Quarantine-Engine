#pragma once
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include "Light.h"
#include "CSMResources.h"
#include <Camera.h>

class DescriptorBuffer;

class DirectionalLight : public Light
{
private:
    float cascadeSplitLambda = 0.95f;
    QECamera* camera = nullptr;

public:
    std::shared_ptr<CSMResources> shadowMappingResourcesPtr = nullptr;

public:
    DirectionalLight();
    DirectionalLight(std::shared_ptr<VkRenderPass> renderPass, QECamera* camera);
    void UpdateUniform() override;
    void CleanShadowMapResources();
    void UpdateCascades();
};

#endif
