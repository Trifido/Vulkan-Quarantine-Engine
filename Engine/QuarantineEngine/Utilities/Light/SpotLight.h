#pragma once
#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H

#include "Light.h"

class SpotLight : public Light
{
public:
    SpotLight();
    void UpdateUniform() override;
};

#endif
