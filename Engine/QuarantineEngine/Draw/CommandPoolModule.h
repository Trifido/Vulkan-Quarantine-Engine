#pragma once
#ifndef COMMAND_POOL_MODULE_H
#define COMMAND_POOL_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "GeometryModule.h"
#include "GraphicsPipelineModule.h"

class BufferManageModule;

class CommandPoolModule
{
private:
    static CommandPoolModule* instance;
    DeviceModule* deviceModule;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

public:
    CommandPoolModule();
    static CommandPoolModule* getInstance();
    VkCommandPool& getCommandPool() { return commandPool; }
    std::vector<VkCommandBuffer>& getCommandBuffers() { return commandBuffers; }
    uint32_t getNumCommandBuffers() { return static_cast<uint32_t>(commandBuffers.size()); }
    VkCommandBuffer& getCommandBuffer(uint32_t idx) { return commandBuffers.at(idx); }
    void createCommandPool(VkSurfaceKHR& surface);
    void createCommandBuffers(std::vector<VkFramebuffer>& swapChainFramebuffers, VkRenderPass& renderPass,
        VkExtent2D& swapChainExtent, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,GeometryModule& geometryModule,
        BufferManageModule& bufferManagerModule, std::vector<VkDescriptorSet>& descriptorSets);
    void cleanup();
};

#endif
