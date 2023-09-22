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

    this->computeNodeEmitParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("emit_compute_particles"));
    this->computeNodeEmitParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("emit_compute_particles", this->computeNodeEmitParticles);

    this->computeNodeUpdateParticles = std::make_shared<ComputeNode>(shaderManager->GetShader("update_compute_particles"));
    this->computeNodeUpdateParticles->computeDescriptor->IsProgressiveComputation = true;
    this->computeNodeManager->AddComputeNode("update_compute_particles", this->computeNodeUpdateParticles);

    this->createShaderStorageBuffers();
    this->InitializeDeadList();

    auto mat = this->materialManager->GetMaterial("defaultParticlesMat");

    auto newMatInstance = mat->CreateMaterialInstance();
    this->materialManager->AddMaterial("defaultParticlesMat", newMatInstance);

    this->addMaterial(newMatInstance);
    this->material->InitializeMaterialDataUBO();
}

void ParticleSystem::cleanup()
{
    GameObject::cleanup();
}

void ParticleSystem::createShaderStorageBuffers()
{
    this->computeNodeEmitParticles->computeDescriptor->InitializeSSBOData();
    //Create particle dead list
    this->computeNodeEmitParticles->computeDescriptor->ssboData.push_back(std::make_shared<UniformBufferObject>());
    this->computeNodeEmitParticles->computeDescriptor->ssboSize.push_back(VkDeviceSize());

    this->computeNodeEmitParticles->NElements = this->MaxNumParticles;
    this->computeNodeEmitParticles->InitializeComputeBuffer(0, sizeof(ParticleV2Input) * this->MaxNumParticles);
    this->computeNodeEmitParticles->InitializeComputeBuffer(1, sizeof(uint32_t) * (this->MaxNumParticles + 1));

    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[0], this->computeNodeEmitParticles->computeDescriptor->ssboSize[0]);
    this->computeNodeUpdateParticles->computeDescriptor->AssignSSBO(this->computeNodeEmitParticles->computeDescriptor->ssboData[1], this->computeNodeEmitParticles->computeDescriptor->ssboSize[1]);
    this->computeNodeUpdateParticles->NElements = this->MaxNumParticles;
    this->computeNodeUpdateParticles->UseDependencyBuffer = true;
}

void ParticleSystem::CreateDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetCullMode(commandBuffer, false);

    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNodeUpdateParticles->computeDescriptor->ssboData[0]->uniformBuffers.at(idx), offsets);

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

void ParticleSystem::InitializeDeadList()
{
    this->deadParticleListSSBO.numDeadParticles = this->MaxNumParticles;
    this->deadParticleListSSBO.deadParticles.reserve(this->MaxNumParticles);

    for (int p = 0; p < this->MaxNumParticles; p++)
    {
        this->deadParticleListSSBO.deadParticles.push_back(p);
    }

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[1]->uniformBuffersMemory[currentFrame], 0, this->computeNodeEmitParticles->computeDescriptor->ssboSize[1], 0, &data);
        memcpy(data, static_cast<const void*>(&this->deadParticleListSSBO), this->computeNodeEmitParticles->computeDescriptor->ssboSize[1]);
        vkUnmapMemory(deviceModule->device, this->computeNodeEmitParticles->computeDescriptor->ssboData[1]->uniformBuffersMemory[currentFrame]);
    }
}

void ParticleSystem::GenerateParticles(size_t currentFrame)
{
    float particlesToCreate = this->ParticlesPerSecond * this->timer->DeltaTime;
    int count = (int)floor(particlesToCreate);
    int entero = (int)particlesToCreate;
    float partialParticle = particlesToCreate - entero;


}

void ParticleSystem::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->CreateDrawCommand(commandBuffer, idx);
}

bool ParticleSystem::IsValid()
{
    return true;
}
