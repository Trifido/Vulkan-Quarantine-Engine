#pragma once

#ifndef LIGHT_TYPE_H
#define LIGHT_TYPE_H

enum LightType
{
    POINT_LIGHT = 0,
    DIRECTIONAL_LIGHT = 1 << 0,
    SPOT_LIGHT = 2 << 0,
    AREA_LIGHT = 3 << 0,
    SUN_LIGHT = 4 << 0,
};

#endif
