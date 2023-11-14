#pragma once

#ifndef PARTICLE_SYSTEM_MANAGER
#define PARTICLE_SYSTEM_MANAGER

#include <unordered_map>
#include <string>
#include <memory>
#include <Particles/ParticleSystem.h>
#include <GameObjectManager.h>

class ParticleSystemManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ParticleSystem>> _particleSystems;
public:
    static ParticleSystemManager* instance;
    GameObjectManager* gameObjectManager;

private:
    std::string CheckName(std::string nameParticleSystem);

public:
    ParticleSystemManager();
    void AddParticleSystem(std::shared_ptr<ParticleSystem> object_ptr, std::string name);
    std::shared_ptr<ParticleSystem> ParticleSystemManager::GetParticleSystem(std::string name);
    static ParticleSystemManager* getInstance();
    static void ResetInstance();
    void Cleanup();
    void CleanLastResources();
    void UpdateParticleSystems();
};

#endif
