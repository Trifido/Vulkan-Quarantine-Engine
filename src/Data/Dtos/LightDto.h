#pragma once
#ifndef LIGHT_DTO_H
#define LIGHT_DTO_H

#include <glm/glm.hpp>
#include <LightType.h>
#include <string>

#pragma pack(1)
struct LightDto
{
    std::string name;
    LightType lightType;
    float radius;
    glm::mat4 worldTransform;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float cutOff;
    float outerCutoff;

    LightDto()
        : name(),
        lightType(LightType::POINT_LIGHT),
        radius{ 1.0f },
        worldTransform(1.0f),
        diffuse{ 0.0f },
        specular{ 0.0f },
        cutOff(0.0f),
        outerCutoff(0.0f)
    {
    }

    LightDto(std::string name, LightType lightType, float radius, glm::mat4 worldTransform, glm::vec3 diffuse, glm::vec3 specular, float cutOff, float outerCutoff)
    {
        this->name = name;
        this->lightType = lightType;
        this->radius = radius;
        this->worldTransform = worldTransform;
        this->diffuse = diffuse;
        this->specular = specular;
        this->cutOff = cutOff;
        this->outerCutoff = outerCutoff;
    }
};

#pragma pack()
#endif
