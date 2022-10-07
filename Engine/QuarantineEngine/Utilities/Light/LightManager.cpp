#include "LightManager.h"
#include <algorithm>

LightManager* LightManager::instance = nullptr;

LightManager::LightManager()
{
    this->lightManagerUniform = std::make_shared<LightManagerUniform>();
}


LightManager* LightManager::getInstance()
{
    if (instance == NULL)
        instance = new LightManager();
    else
        std::cout << "Getting existing instance of Light Manager" << std::endl;

    return instance;
}

void LightManager::CreateLight(LightType type, std::string name)
{
    switch (type)
    {
    default:
    case LightType::POINT_LIGHT:
        this->AddLight(std::static_pointer_cast<Light>(std::make_shared<PointLight>()), name);
        break;
    }
}

std::shared_ptr<Light> LightManager::GetLight(std::string name)
{
    auto it =_lights.find(name);

    if (it != _lights.end())
        return it->second;

    return nullptr;
}

void LightManager::UpdateUniform()
{
    this->lightManagerUniform->numLights = std::min((int)_lights.size(), (int)this->maxNumLights);

    int cont = 0;
    for (auto& it : this->_lights)
    {
        it.second->UpdateUniform();

        this->lightManagerUniform->lights[cont] = *it.second->uniform;
        cont++;
    }

    if (cont < this->maxNumLights)
    {
        for (int i = cont; i < this->maxNumLights; i++)
        {
            this->lightManagerUniform->lights[i] = *this->_lights.begin()->second->uniform;
        }
    }
}

void LightManager::AddLight(std::shared_ptr<Light> light_ptr, std::string name)
{
    if (this->_lights.find(name) == _lights.end())
    {
        this->_lights[name] = light_ptr;
    }
    else
    {
        this->_lights[name + "_1"] = light_ptr;
    }

    this->currentNumLights++;
}
