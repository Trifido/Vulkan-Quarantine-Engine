#pragma once

#ifndef RENDER_LAYER
#define RENDER_LAYER

enum class RenderLayer
{
    ENVIRONMENT = 0,
    SOLID = 1,
    TRANSPARENT_LAYER = 2,
    PARTICLES = 2,
    UI = 3,
    DEBUG = 98,
    EDITOR = 99
};

#endif
