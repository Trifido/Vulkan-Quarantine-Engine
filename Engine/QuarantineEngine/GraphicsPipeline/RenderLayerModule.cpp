#include "RenderLayerModule.h"
#include <iostream>
#include <algorithm>

RenderLayerModule* RenderLayerModule::instance = nullptr;

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
    return enabledLayers.size();
}

RenderLayerModule::RenderLayerModule()
{
    enabledLayers.resize(5);
    enabledLayers[0] = RenderLayer::SOLID;
    enabledLayers[1] = RenderLayer::TRANSPARENT_LAYER;
    enabledLayers[2] = RenderLayer::UI;
    enabledLayers[3] = RenderLayer::DEBUG;
    enabledLayers[4] = RenderLayer::EDITOR;
}

RenderLayerModule* RenderLayerModule::getInstance()
{
    if (instance == NULL)
        instance = new RenderLayerModule();

    return instance;
}
