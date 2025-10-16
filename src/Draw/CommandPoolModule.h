#pragma once
#ifndef COMMAND_POOL_MODULE_H
#define COMMAND_POOL_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "GraphicsPipelineModule.h"
#include "QEGameObject.h"
#include "../Editor/EditorObjectManager.h"
#include <GameObjectManager.h>
#include <Compute/ComputeNodeManager.h>
#include <OmniShadowResources.h>
#include <FrameBufferModule.h>
#include <AtmosphereSystem.h>
#include <CullingSceneManager.h>
#include <PhysicsModule.h>
#include <QESingleton.h>

class CommandPoolModule : public QESingleton<CommandPoolModule>
{
private:
    friend class QESingleton<CommandPoolModule>; // Permitir acceso al constructor
    DeviceModule*                   deviceModule;
    SwapChainModule*                swapchainModule;
    EditorObjectManager*            editorManager;
    GameObjectManager*              gameObjectManager;
    ComputeNodeManager*             computeNodeManager;
    CullingSceneManager*            cullingSceneManager;
    LightManager*                   lightManager;
    RenderPassModule*               renderPassModule;
    AtmosphereSystem*               atmosphereSystem;
    PhysicsModule*                  physicsModule;

    VkCommandPool                   commandPool;
    VkCommandPool                   computeCommandPool;
    std::vector<VkCommandBuffer>    commandBuffers;
    std::vector<VkCommandBuffer>    computeCommandBuffers;

public:
    glm::vec3 ClearColor;

private:
    void setCustomRenderPass(VkFramebuffer& framebuffer, uint32_t iCBuffer);
    void setDirectionalShadowRenderPass(std::shared_ptr<VkRenderPass> renderPass, uint32_t idDirlight, uint32_t iCBuffer);
    void setOmniShadowRenderPass(std::shared_ptr<VkRenderPass> renderPass, uint32_t idPointlight, uint32_t iCBuffer);
    void updateCubeMapFace(uint32_t faceIdx, std::shared_ptr<VkRenderPass> renderPass, uint32_t idPointlight, VkCommandBuffer commandBuffer, uint32_t iCBuffer);
public:
    CommandPoolModule();

    VkCommandPool&                  getCommandPool() { return this->commandPool; }
    VkCommandPool&                  getComputeCommandPool() { return this->computeCommandPool; }
    std::vector<VkCommandBuffer>&   getCommandBuffers() { return this->commandBuffers; }
    std::vector<VkCommandBuffer>&   getComputeCommandBuffers() { return this->computeCommandBuffers; }
    uint32_t                        getNumCommandBuffers() { return static_cast<uint32_t>(this->commandBuffers.size()); }
    VkCommandBuffer&                getCommandBuffer(uint32_t idx) { return this->commandBuffers.at(idx); }
    VkCommandBuffer&                getComputeCommandBuffer(uint32_t idx) { return this->computeCommandBuffers.at(idx); }

    void createCommandPool(VkSurfaceKHR& surface);
    void createCommandBuffers();
    void recreateCommandBuffers();
    void Render(FramebufferModule* framebufferModule);
    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void cleanup();
    void CleanLastResources();
};

#endif
