#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include "Transform.h"
#include "DeviceModule.h"

enum LightType
{
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    SPOT_LIGHT,
    AREA_LIGHT
};

struct AttenuationData
{
    float distance;
    float Linear;
    float Quadratic;
};

const uint32_t NUM_ATTENUATIONS = 12;

class Light
{
protected:
    DeviceModule* deviceModule = nullptr;
    float constant;
    float linear;
    float quadratic;
    float radius;

public:
    std::unique_ptr<Transform> transform;
    std::shared_ptr<LightUniform> uniform;
    uint32_t lightType;
    glm::vec3 diffuse;
    glm::vec3 specular;
    uint32_t idxShadowMap;
    float cutOff;
    float outerCutoff;

public:
    Light();
    virtual void UpdateUniform();
    void SetDistanceEffect(float radiusEffect);
};

#endif
