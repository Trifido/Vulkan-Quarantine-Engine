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

struct LiveBufferInfo
{
    std::string owner;
    VkDeviceSize size = 0;
    VkBufferUsageFlags usage = 0;
};

inline std::unordered_map<uint64_t, FreedHandleInfo> g_freedMemory;
inline std::mutex g_freedMemoryMutex;
inline std::unordered_map<uint64_t, LiveBufferInfo> g_liveBuffers;
inline std::mutex g_liveBuffersMutex;

inline void QETrackBufferAllocation(
    VkBuffer buffer,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    const char* owner)
{
    if (buffer == VK_NULL_HANDLE)
    {
        return;
    }

    const uint64_t handleValue = reinterpret_cast<uint64_t>(buffer);
    std::lock_guard<std::mutex> lock(g_liveBuffersMutex);
    g_liveBuffers[handleValue] = LiveBufferInfo{
        owner != nullptr ? owner : "Unknown",
        size,
        usage
    };
}

inline void QEReportLiveBuffers()
{
    std::lock_guard<std::mutex> lock(g_liveBuffersMutex);
    for (const auto& [handle, info] : g_liveBuffers)
    {
        QE_LOG_ERROR_CAT_F(
            "VulkanLeak",
            "Live VkBuffer handle=0x{:X} owner='{}' size={} usage=0x{:X}",
            handle,
            info.owner,
            static_cast<unsigned long long>(info.size),
            static_cast<uint32_t>(info.usage));
    }
    g_liveBuffers.clear();
}

inline void QETrackMemoryAllocation(
    VkDeviceMemory memory,
    const char* owner,
    const char* file,
    int line)
{
    if (memory == VK_NULL_HANDLE)
    {
        return;
    }

    const uint64_t handleValue = reinterpret_cast<uint64_t>(memory);

    std::lock_guard<std::mutex> lock(g_freedMemoryMutex);
    auto it = g_freedMemory.find(handleValue);
    if (it != g_freedMemory.end())
    {
        g_freedMemory.erase(it);
    }
}

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

    {
        const uint64_t handleValue = reinterpret_cast<uint64_t>(buffer);
        std::lock_guard<std::mutex> lock(g_liveBuffersMutex);
        g_liveBuffers.erase(handleValue);
    }

    vkDestroyBuffer(device, buffer, nullptr);
    buffer = VK_NULL_HANDLE;
}

#define QE_FREE_MEMORY(device, memory, owner) \
    QEFreeMemoryTracked(device, memory, owner, __FILE__, __LINE__)

#define QE_DESTROY_BUFFER(device, buffer, owner) \
    QEDestroyBufferTracked(device, buffer, owner, __FILE__, __LINE__)

#define QE_TRACK_MEMORY_ALLOCATION(memory, owner) \
    QETrackMemoryAllocation(memory, owner, __FILE__, __LINE__)

#define QE_TRACK_BUFFER_ALLOCATION(buffer, size, usage, owner) \
    QETrackBufferAllocation(buffer, size, usage, owner)


namespace QE
{
    using ::FreedHandleInfo;
    using ::LiveBufferInfo;
} // namespace QE
// QE namespace aliases
