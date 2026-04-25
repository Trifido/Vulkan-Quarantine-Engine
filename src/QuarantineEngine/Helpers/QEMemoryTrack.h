#pragma once
#include <vulkan/vulkan.h>
#include <Logging/QELogMacros.h>

#include <unordered_map>
#include <string>
#include <mutex>

struct FreedHandleInfo
{
    std::string owner;
    std::string file;
    int line = 0;
};

static std::unordered_map<uint64_t, FreedHandleInfo> g_freedMemory;
static std::mutex g_freedMemoryMutex;

inline void QEFreeMemoryTracked(
    VkDevice device,
    VkDeviceMemory& memory,
    const char* owner,
    const char* file,
    int line)
{
    if (memory == VK_NULL_HANDLE)
    {
        QE_LOG_WARN_CAT_F("VulkanFree",
            "[{}] vkFreeMemory skipped (already null) at {}:{}",
            owner, file, line);
        return;
    }

    const uint64_t handleValue = reinterpret_cast<uint64_t>(memory);

    {
        std::lock_guard<std::mutex> lock(g_freedMemoryMutex);
        auto it = g_freedMemory.find(handleValue);
        if (it != g_freedMemory.end())
        {
            QE_LOG_ERROR_CAT_F(
                "VulkanFree",
                "[{}] DOUBLE FREE DETECTED for handle=0x{:X} at {}:{} | first freed by [{}] at {}:{}",
                owner,
                handleValue,
                file,
                line,
                it->second.owner,
                it->second.file,
                it->second.line);
        }
        else
        {
            g_freedMemory[handleValue] = FreedHandleInfo{ owner, file, line };
        }
    }

    //QE_LOG_INFO_CAT_F(
    //    "VulkanFree",
    //    "[{}] vkFreeMemory handle=0x{:X} at {}:{}",
    //    owner,
    //    handleValue,
    //    file,
    //    line);

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

    //QE_LOG_INFO_CAT_F("VulkanFree", "[{}] vkDestroyBuffer handle={} at {}:{}",
    //    owner, reinterpret_cast<uint64_t>(buffer), file, line);

    vkDestroyBuffer(device, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
}

#define QE_FREE_MEMORY(device, memory, owner) \
    QEFreeMemoryTracked(device, memory, owner, __FILE__, __LINE__)

#define QE_DESTROY_BUFFER(device, buffer, owner) \
    QEDestroyBufferTracked(device, buffer, owner, __FILE__, __LINE__)


namespace QE
{
    using ::FreedHandleInfo;
} // namespace QE
// QE namespace aliases
