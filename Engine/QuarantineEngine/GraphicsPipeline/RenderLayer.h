#pragma once

#ifndef RENDER_LAYER
#define RENDER_LAYER

enum class RenderLayer
{
    SOLID = 0,
    TRANSPARENT_LAYER = 1,
    PARTICLES = 1,
    UI = 3,
    DEBUG = 98,
    EDITOR = 99
};

#endif
