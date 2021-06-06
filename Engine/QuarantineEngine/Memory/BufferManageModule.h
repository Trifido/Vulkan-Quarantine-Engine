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

#include "Mesh.h"
#include "Transform.h"
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
    QueueModule*        queueModule;
    CommandPoolModule*  commandPoolInstance;
    size_t              numSwapchainImages;
    std::shared_ptr<Mesh>   geoModule;

public:
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

public:
    BufferManageModule();
    void addGeometryData(std::shared_ptr<Mesh> geometryModule);
    std::shared_ptr<Mesh> getGeometryData();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers(size_t numImagesSwapChain);
    static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, DeviceModule& deviceModule);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void updateUniformBuffer(uint32_t currentImage, VkExtent2D extent, std::shared_ptr<Transform> transform);
    void updateUniformBufferCamera(uint32_t currentImage, VkExtent2D extent, Camera& camera);
    void cleanup();
    void cleanupDescriptorBuffer();
};

#endif
