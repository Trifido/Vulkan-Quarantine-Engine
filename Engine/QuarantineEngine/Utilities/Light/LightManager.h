#pragma once
#ifndef LIGHT_MANAGER_H
#define LIGHT_MANAGER_H

#include <unordered_map>
#include <memory>
#include <Light/Light.h>

class LightManager
{
private:
    uint32_t maxNumLights = 8;
    uint32_t currentNumLights = 0;
    std::unordered_map<std::string, std::shared_ptr<Light>> _lights;
public:
    static LightManager* instance;
    std::shared_ptr<LightManagerUniform> lightManagerUniform;

private:
    void AddLight(std::shared_ptr<Light> light_ptr, std::string name);
public:
    static LightManager* getInstance();
    LightManager();
    void CreateLight(LightType type, std::string name);
    std::shared_ptr<Light> GetLight(std::string name);
    void UpdateUniform();
};

#endif
