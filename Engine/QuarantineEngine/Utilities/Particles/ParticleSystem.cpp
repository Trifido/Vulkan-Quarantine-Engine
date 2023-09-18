#include "ParticleSystem.h"
#include "SynchronizationModule.h"
#include "Vertex.h"
#include <random>
#include <filesystem>
#include <ShaderManager.h>
#include <SwapChainModule.h>
#include <BufferManageModule.h>

ParticleSystem::ParticleSystem() : GameObject()
{
    this->timer = Timer::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();
    swapchainModule = SwapChainModule::getInstance();
    auto shaderManager = ShaderManager::getInstance();

    this->computeNode = std::make_shared<ComputeNode>(shaderManager->GetShader("default_compute_particles"));
    this->computeNode->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("default_compute", this->computeNode);

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
    vkDestroyBuffer(deviceModule->device, stagingBufferSSBO1, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferSSBO1Memory, nullptr);
    vkDestroyBuffer(deviceModule->device, stagingBufferSSBO2, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferSSBO2Memory, nullptr);
}

void ParticleSystem::createShaderStorageBuffers()
{
    SwapChainModule* swapchain = SwapChainModule::getInstance();
    // Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(this->MaxNumParticles);
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
    this->computeNode->FillComputeBuffer(this->MaxNumParticles, sizeof(Particle), particles.data());

    this->stagingbufferSize = sizeof(Particle) * this->MaxNumParticles;
    BufferManageModule::createBuffer(stagingbufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        this->stagingBufferSSBO1, this->stagingBufferSSBO1Memory, *deviceModule);
    BufferManageModule::createBuffer(stagingbufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        this->stagingBufferSSBO2, this->stagingBufferSSBO2Memory, *deviceModule);

    vkMapMemory(this->deviceModule->device, stagingBufferSSBO1Memory, 0, stagingbufferSize, 0, &dataSSBO1);
    vkUnmapMemory(this->deviceModule->device, stagingBufferSSBO1Memory);
    vkMapMemory(this->deviceModule->device, stagingBufferSSBO2Memory, 0, stagingbufferSize, 0, &dataSSBO2);
    vkUnmapMemory(this->deviceModule->device, stagingBufferSSBO2Memory);

    this->tempSSBO1 = new Particle[this->MaxNumParticles];
    this->tempSSBO2 = new Particle[this->MaxNumParticles];
}

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetCullMode(commandBuffer, false);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNode->computeDescriptor->ssboData[0]->uniformBuffers.at(idx), offsets);

    if (this->material->HasDescriptorBuffer())
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material->descriptor->getDescriptorSet(idx), 0, nullptr);
    }

    this->pushConstant.model = this->transform->GetModel();
    if (this->parent != nullptr)
    {
        pushConstant.model = this->parent->transform->GetModel() * pushConstant.model;
    }
    vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantStruct), &pushConstant);

    vkCmdDraw(commandBuffer, this->MaxNumParticles, 1, 0, 0);
}

void ParticleSystem::GenerateParticles(size_t currentFrame)
{
    float particlesToCreate = this->ParticlesPerSecond * this->timer->DeltaTime;
    int count = (int)floor(particlesToCreate);
    int entero = (int)particlesToCreate;
    float partialParticle = particlesToCreate - entero;


    BufferManageModule::copyBuffer(this->computeNode->computeDescriptor->ssboData[0]->uniformBuffers[0], this->stagingBufferSSBO1, this->computeNode->computeDescriptor->ssboSize[0], *deviceModule);
    BufferManageModule::copyBuffer(this->computeNode->computeDescriptor->ssboData[0]->uniformBuffers[1], this->stagingBufferSSBO2, this->computeNode->computeDescriptor->ssboSize[0], *deviceModule);

    memcpy(tempSSBO1, dataSSBO1, stagingbufferSize);
    memcpy(tempSSBO2, dataSSBO2, stagingbufferSize);

    BufferManageModule::copyBuffer(this->stagingBufferSSBO1, this->computeNode->computeDescriptor->ssboData[0]->uniformBuffers[0],  this->computeNode->computeDescriptor->ssboSize[0], *deviceModule);
    BufferManageModule::copyBuffer(this->stagingBufferSSBO2, this->computeNode->computeDescriptor->ssboData[0]->uniformBuffers[1],  this->computeNode->computeDescriptor->ssboSize[0], *deviceModule);

    if (this->currentParticlesNum < this->MaxNumParticles)
    {

    }

    if (this->InfinityLifeTime)
    {
        for (int i = 0; i < count; i++) {
            EmitParticle();
        }
    }
    else if (this->currentLifeTime < this->LifeTime)
    {
        for (int i = 0; i < count; i++) {
            EmitParticle();
        }
        this->currentLifeTime += this->timer->DeltaTime;
    }

}

void ParticleSystem::EmitParticle()
{
    glm::vec3 emitCone = glm::vec3(0.0, 1.0, 0.0);
    if (this->EmissionAngle > 0.0f)
    {
        float dirX = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * this->EmissionAngle - (this->EmissionAngle / 2.0f);
        float dirZ = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * this->EmissionAngle - (this->EmissionAngle / 2.0f);
        emitCone = glm::vec3(dirX, 1.0, dirZ);
    }

    glm::vec3 velocity = glm::normalize(emitCone);
    velocity *= this->Speed;

    glm::vec3 emitCenter = this->transform->Position;

    if (this->EmissionRadius > 0.0f)
    {
        float xPos = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * this->EmissionRadius - (this->EmissionRadius / 2.0f);
        float zPos = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * this->EmissionRadius - (this->EmissionRadius / 2.0f);
        emitCenter += glm::vec3(xPos, 0.0, zPos);
    }
}

void ParticleSystem::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->CreateDrawCommand(commandBuffer, idx);
}

bool ParticleSystem::IsValid()
{
    return true;
}
