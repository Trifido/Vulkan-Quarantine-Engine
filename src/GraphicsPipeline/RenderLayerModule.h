#pragma once

#ifndef RENDER_LAYER_MODULE
#define RENDER_LAYER_MODULE

#include <RenderLayer.h>
#include <vector>

class RenderLayerModule
{
private:
    std::vector<RenderLayer> enabledLayers;

public:
    RenderLayerModule();
    void AddLayer(RenderLayer newLayer);
    unsigned int GetLayer(unsigned int idx);
    unsigned int GetCount();
};

#endif // !RENDER_LAYER_MODULE


