#pragma once
#ifndef COMMAND_POOL_MODULE_H
#define COMMAND_POOL_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "GraphicsPipelineModule.h"
#include "GameObject.h"
#include "../Editor/EditorObjectManager.h"
#include <GameObjectManager.h>
#include <Compute/ComputeNodeManager.h>

class CommandPoolModule
{
private:
    static CommandPoolModule*       instance;
    DeviceModule*                   deviceModule;
    SwapChainModule*                swapchainModule;
    EditorObjectManager*            editorManager;
    GameObjectManager*              gameObjectManager;
    ComputeNodeManager*             computeNodeManager;

    VkCommandPool                   commandPool;
    VkCommandPool                   computeCommandPool;
    std::vector<VkCommandBuffer>    commandBuffers;
    std::vector<VkCommandBuffer>    computeCommandBuffers;

public:
    glm::vec3 ClearColor;

public:
    CommandPoolModule();
    static CommandPoolModule* getInstance();
    static void ResetInstance();

    VkCommandPool&                  getCommandPool() { return this->commandPool; }
    VkCommandPool&                  getComputeCommandPool() { return this->computeCommandPool; }
    std::vector<VkCommandBuffer>&   getCommandBuffers() { return this->commandBuffers; }
    std::vector<VkCommandBuffer>&   getComputeCommandBuffers() { return this->computeCommandBuffers; }
    uint32_t                        getNumCommandBuffers() { return static_cast<uint32_t>(this->commandBuffers.size()); }
    VkCommandBuffer&                getCommandBuffer(uint32_t idx) { return this->commandBuffers.at(idx); }

    void bindComputeNodeManager();
    void createCommandPool(VkSurfaceKHR& surface);
    void createCommandBuffers();
    void Render(VkFramebuffer& swapChainFramebuffer, VkRenderPass& renderPass);
    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void cleanup();
    void CleanLastResources();
};

#endif
