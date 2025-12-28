#pragma once
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include "Light.h"
#include "CSMResources.h"

class DescriptorBuffer;

class QEDirectionalLight : public QELight
{
    REFLECTABLE_DERIVED_COMPONENT(QEDirectionalLight, QELight)

private:
    REFLECT_PROPERTY(float, cascadeSplitLambda)

public:
    std::shared_ptr<CSMResources> shadowMappingResourcesPtr = nullptr;

public:
    QEDirectionalLight();
    void Setup(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
    void UpdateCascades();
};

#endif
