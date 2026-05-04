#pragma once
#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H

#include "Light.h"
#include <SpotShadowResources.h>

class QESpotLight : public QELight
{
    REFLECTABLE_DERIVED_COMPONENT(QESpotLight, QELight)
public:
    std::shared_ptr<SpotShadowResources> shadowMappingResourcesPtr = nullptr;

public:
    QESpotLight();
    void Setup(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
};



namespace QE
{
    using ::QESpotLight;
} // namespace QE
// QE namespace aliases
#endif
