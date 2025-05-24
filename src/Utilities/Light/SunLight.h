#pragma once
#ifndef SUN_LIGHT_H
#define SUN_LIGHT_H

#include "DirectionalLight.h"

class SunLight : public DirectionalLight
{
public:
    SunUniform uniformData;
    std::shared_ptr<UniformBufferObject> sunUBO = nullptr;
    float baseIntensity;

public:
    SunLight();
    SunLight(std::shared_ptr<VkRenderPass> renderPass, QECamera* camera);
    void UpdateSun();
    void SetLightDirection(glm::vec3 dir);
};

#endif // !SUN_LIGHT_H
