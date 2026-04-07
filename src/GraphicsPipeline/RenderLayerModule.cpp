#include "RenderLayerModule.h"
#include <iostream>
#include <algorithm>
#include <QELogMacros.h>

void RenderLayerModule::AddLayer(RenderLayer newLayer)
{
    enabledLayers.push_back(newLayer);
    std::sort(enabledLayers.begin(), enabledLayers.end());
}

unsigned int RenderLayerModule::GetLayer(unsigned int idx) const
{
    if (idx < enabledLayers.size())
    {
        return (unsigned int) this->enabledLayers.at(idx);
    }

    QE_LOG_ERROR_CAT("RenderLayerModule", "Layer index not found");
    return 0;
}

unsigned int RenderLayerModule::GetCount() const
{
    return static_cast<unsigned int>(enabledLayers.size());
}

RenderLayerModule::RenderLayerModule()
{
    enabledLayers.resize(7);
    enabledLayers[0] = RenderLayer::ENVIRONMENT;
    enabledLayers[1] = RenderLayer::SOLID;
    enabledLayers[2] = RenderLayer::TRANSPARENT_LAYER;
    enabledLayers[3] = RenderLayer::PARTICLES;
    enabledLayers[4] = RenderLayer::UI;
    enabledLayers[5] = RenderLayer::DEBUG_LAYER;
    enabledLayers[6] = RenderLayer::EDITOR;
}
