#pragma once
#ifndef COMMAND_POOL_MODULE_H
#define COMMAND_POOL_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "GraphicsPipelineModule.h"
#include "GameObject.h"
#include "../Editor/EditorObjectManager.h"
#include <GameObjectManager.h>

class CommandPoolModule
{
private:
    static CommandPoolModule*       instance;
    DeviceModule*                   deviceModule;
    SwapChainModule*                swapchainModule;
    EditorObjectManager*            editorManager;
    GameObjectManager*              gameObjectManager;

    VkCommandPool                   commandPool;
    std::vector<VkCommandBuffer>    commandBuffers;

public:
    glm::vec3 ClearColor;

public:
    CommandPoolModule();
    static CommandPoolModule* getInstance();

    VkCommandPool&                  getCommandPool() { return commandPool; }
    std::vector<VkCommandBuffer>&   getCommandBuffers() { return commandBuffers; }
    uint32_t                        getNumCommandBuffers() { return static_cast<uint32_t>(commandBuffers.size()); }
    VkCommandBuffer&                getCommandBuffer(uint32_t idx) { return commandBuffers.at(idx); }

    void createCommandPool(VkSurfaceKHR& surface);
    void createCommandBuffers();
    void Render(std::vector<VkFramebuffer>& swapChainFramebuffers, VkRenderPass& renderPass);
    void cleanup();
};

#endif
