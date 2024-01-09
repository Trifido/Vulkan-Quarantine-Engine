#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include "Transform.h"

enum LightType
{
    POINT_LIGHT,
    DIRECTIONAL_LIGHT,
    SPOT_LIGHT,
    AREA_LIGHT
};

class Light
{
public:
    std::unique_ptr<Transform> transform;
public:
    std::shared_ptr<LightUniform> uniform;

    uint32_t lightType;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float spotCutOff;
    float spotExponent;
    float radius;
    uint32_t idxShadowMap;

public:
    Light();
    virtual void UpdateUniform();
};

#endif
