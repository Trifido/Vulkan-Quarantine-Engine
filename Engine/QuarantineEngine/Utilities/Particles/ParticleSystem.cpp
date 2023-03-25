#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include <random>

void ParticleSystem::createShaderStorageBuffers()
{
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(this->numParticles);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        float x = r * cos(theta) * 800 / 600;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    this->computeNode->FillComputeBuffer(this->numParticles, sizeof(Particle), particles.data());
}

ParticleSystem::ParticleSystem()
{
    GameObject::GameObject();
    this->computeNodeManager = ComputeNodeManager::getInstance();

    std::string name = "default_compute";
    this->computeNodeManager->CreateComputeNode(name, false);
    this->computeNode = this->computeNodeManager->GetComputeNode(name);
    this->numParticles = 200;

    this->createShaderStorageBuffers();
}

void ParticleSystem::cleanup()
{
    GameObject::cleanup();
    this->computeNode->cleanup();
}
