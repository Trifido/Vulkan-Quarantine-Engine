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

#include "GeometryModule.h"
#include "DeviceModule.h"
#include "CommandPoolModule.h"
#include "QueueFamiliesModule.h"
#include "Camera.h"

class CommandPoolModule;

class BufferManageModule
{
private:
    void*               data;
    DeviceModule*       deviceModule;
    CommandPoolModule*  commandPoolInstance;
    size_t              numSwapchainImages;
    GeometryModule*     geoModule;
    QueueModule*        queueModule;
public:
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

public:
    BufferManageModule();
    void addGeometryQueueData(GeometryModule& geometryModule, QueueModule& queueModule);
    GeometryModule* getGeometryData();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers(size_t numImagesSwapChain, VkDeviceSize size);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void updateUniformBuffer(uint32_t currentImage, VkExtent2D extent);
    void updateUniformBufferCamera(uint32_t currentImage, VkExtent2D extent, Camera& camera);
    void cleanup();
    void cleanupDescriptorBuffer();
};

#endif
