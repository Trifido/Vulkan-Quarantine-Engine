#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include "Vertex.h"
#include <random>
#include <filesystem>
#include <ShaderManager.h>
#include <SwapChainModule.h>

ParticleSystem::ParticleSystem() : GameObject()
{
    this->timer = Timer::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();
    swapchainModule = SwapChainModule::getInstance();
    auto shaderManager = ShaderManager::getInstance();

    this->computeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("default_compute_particles"));
    this->computeNode->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("default_compute", this->computeNode);
    this->numParticles = 8192;

    this->createShaderStorageBuffers();

    auto mat = this->materialManager->GetMaterial("defaultParticlesMat");

    auto newMatInstance = mat->CreateMaterialInstance();
    this->materialManager->AddMaterial("defaultParticlesMat", newMatInstance);

    this->addMaterial(newMatInstance);
    this->material->InitializeMaterialDataUBO();
}

void ParticleSystem::cleanup()
{
    GameObject::cleanup();
    this->computeNode->cleanup();
}

void ParticleSystem::createShaderStorageBuffers()
{
    SwapChainModule* swapchain = SwapChainModule::getInstance();
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(this->numParticles);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        float x = r * cos(theta) * swapchain->swapChainExtent.width / swapchain->swapChainExtent.height;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x, y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    this->computeNode->computeDescriptor->InitializeSSBOData();
    this->computeNode->FillComputeBuffer(this->numParticles, sizeof(Particle), particles.data());
}

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, false);
    vkCmdSetCullMode(commandBuffer, false);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNode->computeDescriptor->ssboData[0]->uniformBuffers.at(idx), offsets);

    vkCmdDraw(commandBuffer, this->numParticles, 1, 0, 0);
}

void ParticleSystem::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->CreateDrawCommand(commandBuffer, idx);
}

bool ParticleSystem::IsValid()
{
    return true;
}
