#pragma once
#ifndef BUFFER_MANAGE_MODULE_H
#define BUFFER_MANAGE_MODULE_H

#define GLM_FORCE_RADIANS

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <vector>
#include <array>
#include <memory>

#include "DeviceModule.h"
#include "QueueFamiliesModule.h"

class BufferManageModule
{
public:
    static VkCommandPool    commandPool;
    static VkQueue          graphicsQueue;
    static VkCommandPool    computeCommandPool;
    static VkQueue          computeQueue;

public:
    BufferManageModule();
    static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, DeviceModule& deviceModule);
    static void createSharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, DeviceModule& deviceModule);
    static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, DeviceModule& deviceModule);
    //void updateUniformBufferCamera(uint32_t currentImage, VkExtent2D extent, Camera& camera);
};


#endif
