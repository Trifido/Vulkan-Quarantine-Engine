#pragma once
#include <vulkan/vulkan.h>
#include <Logging/QELogMacros.h>

inline void QEFreeMemoryTracked(
    VkDevice device,
    VkDeviceMemory& memory,
    const char* owner,
    const char* file,
    int line)
{
    if (memory == VK_NULL_HANDLE)
    {
        QE_LOG_WARN_CAT_F("VulkanFree", "[{}] vkFreeMemory skipped (already null) at {}:{}",
            owner, file, line);
        return;
    }

    QE_LOG_INFO_CAT_F("VulkanFree", "[{}] vkFreeMemory handle={} at {}:{}",
        owner, reinterpret_cast<uint64_t>(memory), file, line);

    vkFreeMemory(device, memory, nullptr);
    memory = VK_NULL_HANDLE;
}

inline void QEDestroyBufferTracked(
    VkDevice device,
    VkBuffer& buffer,
    const char* owner,
    const char* file,
    int line)
{
    if (buffer == VK_NULL_HANDLE)
    {
        QE_LOG_WARN_CAT_F("VulkanFree", "[{}] vkDestroyBuffer skipped (already null) at {}:{}",
            owner, file, line);
        return;
    }

    QE_LOG_INFO_CAT_F("VulkanFree", "[{}] vkDestroyBuffer handle={} at {}:{}",
        owner, reinterpret_cast<uint64_t>(buffer), file, line);

    vkDestroyBuffer(device, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
}

#define QE_FREE_MEMORY(device, memory, owner) \
    QEFreeMemoryTracked(device, memory, owner, __FILE__, __LINE__)

#define QE_DESTROY_BUFFER(device, buffer, owner) \
    QEDestroyBufferTracked(device, buffer, owner, __FILE__, __LINE__)
