#pragma once
#ifndef BUFFER_MANAGE_MODULE_H
#define BUFFER_MANAGE_MODULE_H

#define GLM_FORCE_RADIANS

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <vector>
#include <array>

#include "GeometryModule.h"
#include "DeviceModule.h"
#include "CommandPoolModule.h"
#include "QueueFamiliesModule.h"

class CommandPoolModule;

class BufferManageModule
{
private:
    void*               data;
    DeviceModule*       deviceModule;
    CommandPoolModule*  commandPoolInstance;
    size_t              numSwapchainImages;
public:
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

public:
    BufferManageModule();
    void createVertexBuffer(GeometryModule& geoModule, QueueModule& queueModule);
    void createIndexBuffer(GeometryModule& geoModule, QueueModule& queueModule);
    void createUniformBuffers(size_t numImagesSwapChain);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, QueueModule& queueModule);
    void updateUniformBuffer(uint32_t currentImage, VkExtent2D extent);
    void cleanup();
    void cleanupDescriptorBuffer();
};

#endif
