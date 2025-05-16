#pragma once

#ifndef PARTICLE_SYSTEM_MANAGER
#define PARTICLE_SYSTEM_MANAGER

#include <unordered_map>
#include <string>
#include <memory>
#include <Particles/ParticleSystem.h>
#include <GameObjectManager.h>
#include <QESingleton.h>

class ParticleSystemManager : public QESingleton<ParticleSystemManager>
{
private:
    friend class QESingleton<ParticleSystemManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<ParticleSystem>> _particleSystems;

public:
    GameObjectManager* gameObjectManager;

private:
    std::string CheckName(std::string nameParticleSystem);

public:
    ParticleSystemManager();
    void AddParticleSystem(std::shared_ptr<ParticleSystem> object_ptr, std::string name);
    std::shared_ptr<ParticleSystem> GetParticleSystem(std::string name);
    void Cleanup();
    void CleanLastResources();
    void UpdateParticleSystems();
};

#endif
