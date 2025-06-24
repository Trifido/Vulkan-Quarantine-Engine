#pragma once
#ifndef SUN_LIGHT_H
#define SUN_LIGHT_H

#include "DirectionalLight.h"

class QESunLight : public QEDirectionalLight
{
    REFLECTABLE_DERIVED_COMPONENT(QESunLight, QELight)

public:
    SunUniform uniformData;
    std::shared_ptr<UniformBufferObject> sunUBO = nullptr;

    REFLECT_PROPERTY(float, baseIntensity)

public:
    QESunLight();
    QESunLight(std::shared_ptr<VkRenderPass> renderPass, QECamera* camera);
    void UpdateSun();
    void SetLightDirection(glm::vec3 dir);
};

#endif // !SUN_LIGHT_H
