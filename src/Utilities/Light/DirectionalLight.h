#pragma once
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include "Light.h"
#include "CSMResources.h"
#include <QECamera.h>

class DescriptorBuffer;

class QEDirectionalLight : public QELight
{
    REFLECTABLE_DERIVED_COMPONENT(QEDirectionalLight, QELight)

private:
    REFLECT_PROPERTY(float, cascadeSplitLambda)
    QECamera* camera = nullptr;

public:
    std::shared_ptr<CSMResources> shadowMappingResourcesPtr = nullptr;

public:
    QEDirectionalLight();
    void Setup(std::shared_ptr<VkRenderPass> renderPass, QECamera* camera);
    void UpdateUniform() override;
    void CleanShadowMapResources();
    void UpdateCascades();
};

#endif
