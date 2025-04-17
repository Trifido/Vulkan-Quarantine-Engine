#pragma once
#ifndef SUN_LIGHT_H
#define SUN_LIGHT_H

#include "DirectionalLight.h"

class SunLight : public DirectionalLight
{
public:
    SunUniform uniformData;
    std::shared_ptr<UniformBufferObject> sunUBO = nullptr;

public:
    SunLight();
    SunLight(std::shared_ptr<VkRenderPass> renderPass, Camera* camera);
    void UpdateSun();
    void SetParameters(glm::vec3 dir, float intensity);
};

#endif // !SUN_LIGHT_H
