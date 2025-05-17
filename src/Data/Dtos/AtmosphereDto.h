#pragma once

#ifndef ATMOSPHERE_DTO_H
#define ATMOSPHERE_DTO_H

#include <glm/glm.hpp>

#pragma pack(1)
struct AtmosphereDto
{
    bool hasAtmosphere;
    int environmentType;
    glm::vec3 sunDirection;
    float sunIntensity;

    AtmosphereDto()
        : hasAtmosphere(true),
        environmentType(2),
        sunDirection{ 0.0, -0.1, 0.1 },
        sunIntensity(100.0f)
    {
    }

    AtmosphereDto(bool hasAtmosphere, int environmentType, glm::vec3 sunDirection, float sunIntensity)
    {
        this->hasAtmosphere = hasAtmosphere;
        this->environmentType = environmentType;
        this->sunDirection = sunDirection;
        this->sunIntensity = sunIntensity;
    }
};
#pragma pack()
#endif
