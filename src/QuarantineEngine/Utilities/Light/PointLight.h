#pragma once
#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include <Light.h>
#include <OmniShadowResources.h>

class DescriptorBuffer;

class QEPointLight : public QELight
{
    REFLECTABLE_DERIVED_COMPONENT(QEPointLight, QELight)
public:
    std::shared_ptr<OmniShadowResources> shadowMappingResourcesPtr = nullptr;

public:
    QEPointLight();
    void Setup(std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
};



namespace QE
{
    using ::DescriptorBuffer;
    using ::QEPointLight;
} // namespace QE
// QE namespace aliases
#endif
