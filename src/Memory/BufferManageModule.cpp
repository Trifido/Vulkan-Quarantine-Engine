#include "BufferManageModule.h"
#include <stdexcept>

#include "ImageMemoryTools.h"


VkCommandPool BufferManageModule::commandPool;
VkQueue BufferManageModule::graphicsQueue;
VkCommandPool BufferManageModule::computeCommandPool;
VkQueue BufferManageModule::computeQueue;

BufferManageModule::BufferManageModule()
{
}

void BufferManageModule::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, DeviceModule& deviceModule)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(deviceModule.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(deviceModule.device, buffer, &memRequirements);

    uint32_t memoryTypeIndex = IMT::findMemoryType(memRequirements.memoryTypeBits, properties, deviceModule.physicalDevice);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    allocInfo.pNext = nullptr;

    const bool wantsDeviceAddress = (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0;

    VkMemoryAllocateFlagsInfo allocFlagsInfo{};
    if (wantsDeviceAddress)
    {
        VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeat{};
        bdaFeat.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        VkPhysicalDeviceFeatures2 feats2{};
        feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        feats2.pNext = &bdaFeat;

        vkGetPhysicalDeviceFeatures2(deviceModule.physicalDevice, &feats2);

        if (bdaFeat.bufferDeviceAddress == VK_TRUE)
        {
            allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            allocFlagsInfo.pNext = nullptr;
            allocInfo.pNext = &allocFlagsInfo;
        }
        else
        {
            vkDestroyBuffer(deviceModule.device, buffer, nullptr);
            throw std::runtime_error("Buffer requests VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT but device doesn't have bufferDeviceAddress enabled");
        }
    }

    // Allocate memory
    if (vkAllocateMemory(deviceModule.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(deviceModule.device, buffer, nullptr);
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    // Bind memory
    if (vkBindBufferMemory(deviceModule.device, buffer, bufferMemory, 0) != VK_SUCCESS)
    {
        vkFreeMemory(deviceModule.device, bufferMemory, nullptr);
        vkDestroyBuffer(deviceModule.device, buffer, nullptr);
        throw std::runtime_error("failed to bind buffer memory!");
    }
}


void BufferManageModule::createSharedBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, DeviceModule& deviceModule)
{
    std::vector<uint32_t> queueIndices = { deviceModule.queueIndices.graphicsFamily.value(), deviceModule.queueIndices.computeFamily.value() };

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    bufferInfo.queueFamilyIndexCount = (uint32_t)queueIndices.size();
    bufferInfo.pQueueFamilyIndices = queueIndices.data();

    if (vkCreateBuffer(deviceModule.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(deviceModule.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = IMT::findMemoryType(memRequirements.memoryTypeBits, properties, deviceModule.physicalDevice);

    if (vkAllocateMemory(deviceModule.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(deviceModule.device, buffer, bufferMemory, 0);
}

void BufferManageModule::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, DeviceModule& deviceModule)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(deviceModule.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(deviceModule.device, commandPool, 1, &commandBuffer);
}
