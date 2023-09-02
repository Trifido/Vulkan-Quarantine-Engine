#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include <random>
#include <filesystem>

ParticleSystem::ParticleSystem() : GameObject()
{
    this->computeNodeManager = ComputeNodeManager::getInstance();
    swapchainModule = SwapChainModule::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders/Compute/").generic_string();
    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);
    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_default_compute_shader_path = absPath + "default_compute.spv";
    this->computeNode = std::make_shared<ComputeNode>(absolute_default_compute_shader_path);
    this->computeNodeManager->AddComputeNode("default_compute", this->computeNode);
    this->numParticles = 200;

    this->createShaderStorageBuffers();

    auto mat = this->materialManager->GetMaterial("default_particles");

    auto newMatInstance = mat->CreateMaterialInstance();
    this->materialManager->AddMaterial("default_particlesMat", newMatInstance);

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

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->material->pipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainModule->swapChainExtent.width;
    viewport.height = (float)swapchainModule->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainModule->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNode->computeDescriptor->ssbo->uniformBuffers.at(idx), offsets);

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
