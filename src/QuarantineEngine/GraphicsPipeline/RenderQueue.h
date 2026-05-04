#pragma once

#ifndef RENDER_QUEUE
#define RENDER_QUEUE

enum class RenderQueue
{
    Background = 1000,
    Geometry = 2000,
    Transparent = 3000,
    Particles = 3100,
    UI = 4000,
    Debug = 5000,
    Editor = 6000
};



namespace QE
{
    using ::RenderQueue;
} // namespace QE
// QE namespace aliases
#endif
