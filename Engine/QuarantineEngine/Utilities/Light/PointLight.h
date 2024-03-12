#pragma once
#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include "Light.h"

class PointLight : public Light
{
public:
    PointLight();
    void UpdateUniform() override;
};

#endif
