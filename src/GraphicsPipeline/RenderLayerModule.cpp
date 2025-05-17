#include "RenderLayerModule.h"
#include <iostream>
#include <algorithm>

void RenderLayerModule::AddLayer(RenderLayer newLayer)
{
    enabledLayers.push_back(newLayer);
    std::sort(enabledLayers.begin(), enabledLayers.end());
}

unsigned int RenderLayerModule::GetLayer(unsigned int idx)
{
    if (idx < enabledLayers.size())
    {
        return (unsigned int) this->enabledLayers.at(idx);
    }

    std::cerr << "ERROR: Indice de layer no encontrada\n";
    return 0;
}

unsigned int RenderLayerModule::GetCount()
{
    return static_cast<unsigned int>(enabledLayers.size());
}

RenderLayerModule::RenderLayerModule()
{
    enabledLayers.resize(6);
    enabledLayers[0] = RenderLayer::SOLID;
    enabledLayers[1] = RenderLayer::TRANSPARENT_LAYER;
    enabledLayers[2] = RenderLayer::PARTICLES;
    enabledLayers[3] = RenderLayer::UI;
    enabledLayers[4] = RenderLayer::DEBUG;
    enabledLayers[5] = RenderLayer::EDITOR;
}
