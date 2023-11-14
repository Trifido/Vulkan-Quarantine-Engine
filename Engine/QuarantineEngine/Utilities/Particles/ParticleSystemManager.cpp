#include "ParticleSystemManager.h"

ParticleSystemManager* ParticleSystemManager::instance = nullptr;

ParticleSystemManager::ParticleSystemManager()
{
    this->gameObjectManager = GameObjectManager::getInstance();
}

std::string ParticleSystemManager::CheckName(std::string nameParticleSystem)
{
    std::unordered_map<std::string, std::shared_ptr<ParticleSystem>>::const_iterator got;
    std::string newName = nameParticleSystem;
    unsigned int id = 0;

    do
    {
        got = _particleSystems.find(newName);

        if (got != _particleSystems.end())
        {
            id++;
            newName = nameParticleSystem + "_" + std::to_string(id);
        }
    } while (got != _particleSystems.end());

    return newName;
}

void ParticleSystemManager::AddParticleSystem(std::shared_ptr<ParticleSystem> object_ptr, std::string name)
{
    if (object_ptr->IsValid())
    {
        name = CheckName(name);

        this->_particleSystems[name] = object_ptr;
        this->gameObjectManager->AddGameObject(object_ptr, name);
    }
}

std::shared_ptr<ParticleSystem> ParticleSystemManager::GetParticleSystem(std::string name)
{
    auto it = this->_particleSystems.find(name);

    if (it != this->_particleSystems.end())
        return it->second;

    return nullptr;
}

ParticleSystemManager* ParticleSystemManager::getInstance()
{
    if (instance == NULL)
        instance = new ParticleSystemManager();

    return instance;
}

void ParticleSystemManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

void ParticleSystemManager::Cleanup()
{
    for (auto model : this->_particleSystems)
    {
        model.second->cleanup();
    }
}

void ParticleSystemManager::CleanLastResources()
{
    this->_particleSystems.clear();
    this->gameObjectManager = nullptr;
}

void ParticleSystemManager::UpdateParticleSystems()
{
    for (auto model : this->_particleSystems)
    {
        model.second->Update();
    }
}
