#include "QEGeometryResourceCache.h"

#include <BufferManageModule.h>
#include <DeviceModule.h>
#include <Helpers/QEMemoryTrack.h>
#include <cstring>
#include <stdexcept>

std::unordered_map<std::string, std::weak_ptr<QEGeometrySharedResource>> QEGeometryResourceCache::cache;
std::mutex QEGeometryResourceCache::cacheMutex;

namespace
{
    QEGeometryBufferAllocation CreateBufferAllocation(
        VkDeviceSize bufferSize,
        VkBufferUsageFlags usageFlags,
        const void* dataArray,
        DeviceModule& deviceModule)
    {
        QEGeometryBufferAllocation allocation{};

        if (bufferSize == 0 || dataArray == nullptr)
        {
            return allocation;
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        BufferManageModule::createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory,
            deviceModule);

        void* data = nullptr;
        vkMapMemory(deviceModule.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        std::memcpy(data, dataArray, static_cast<size_t>(bufferSize));
        vkUnmapMemory(deviceModule.device, stagingBufferMemory);

        BufferManageModule::createBuffer(
            bufferSize,
            usageFlags,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            allocation.Buffer,
            allocation.Memory,
            deviceModule);
        BufferManageModule::copyBuffer(stagingBuffer, allocation.Buffer, bufferSize, deviceModule);

        QE_DESTROY_BUFFER(deviceModule.device, stagingBuffer, "QEGeometryResourceCache::CreateBufferAllocation");
        QE_FREE_MEMORY(deviceModule.device, stagingBufferMemory, "QEGeometryResourceCache::CreateBufferAllocation");

        return allocation;
    }

    void DestroyAllocations(std::vector<QEGeometryBufferAllocation>& allocations, VkDevice device, const char* scope)
    {
        for (auto& allocation : allocations)
        {
            if (allocation.Buffer != VK_NULL_HANDLE)
            {
                QE_DESTROY_BUFFER(device, allocation.Buffer, scope);
                allocation.Buffer = VK_NULL_HANDLE;
            }

            if (allocation.Memory != VK_NULL_HANDLE)
            {
                QE_FREE_MEMORY(device, allocation.Memory, scope);
                allocation.Memory = VK_NULL_HANDLE;
            }
        }
    }
}

QEGeometrySharedResource::~QEGeometrySharedResource()
{
    auto* deviceModule = DeviceModule::getInstance();
    if (deviceModule == nullptr || deviceModule->device == VK_NULL_HANDLE)
    {
        return;
    }

    DestroyAllocations(IndexBuffers, deviceModule->device, "QEGeometrySharedResource::~QEGeometrySharedResource");
    DestroyAllocations(VertexBuffers, deviceModule->device, "QEGeometrySharedResource::~QEGeometrySharedResource");
    DestroyAllocations(AnimationBuffers, deviceModule->device, "QEGeometrySharedResource::~QEGeometrySharedResource");
}

std::shared_ptr<QEGeometrySharedResource> QEGeometryResourceCache::Acquire(
    const std::string& key,
    const std::function<QEMesh()>& buildMeshFn)
{
    if (key.empty())
    {
        return CreateResource(buildMeshFn());
    }

    std::lock_guard<std::mutex> lock(cacheMutex);

    CollectGarbage();

    auto it = cache.find(key);
    if (it != cache.end())
    {
        if (auto existing = it->second.lock())
        {
            return existing;
        }
    }

    auto resource = CreateResource(buildMeshFn());
    cache[key] = resource;
    return resource;
}

void QEGeometryResourceCache::CollectGarbage()
{
    for (auto it = cache.begin(); it != cache.end();)
    {
        if (it->second.expired())
        {
            it = cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<QEGeometrySharedResource> QEGeometryResourceCache::CreateResource(const QEMesh& mesh)
{
    auto* deviceModule = DeviceModule::getInstance();
    if (deviceModule == nullptr)
    {
        throw std::runtime_error("QEGeometryResourceCache requires a valid DeviceModule");
    }

    auto resource = std::make_shared<QEGeometrySharedResource>();
    resource->Mesh = mesh;

    const size_t subMeshCount = mesh.MeshData.size();
    resource->VertexBuffers.resize(subMeshCount);
    resource->IndexBuffers.resize(subMeshCount);
    resource->AnimationBuffers.resize(subMeshCount);
    resource->Meshlets.resize(subMeshCount);

    for (size_t i = 0; i < subMeshCount; ++i)
    {
        const auto& subMesh = mesh.MeshData[i];

        if (!subMesh.Vertices.empty())
        {
            const VkDeviceSize vertexBufferSize = sizeof(subMesh.Vertices[0]) * subMesh.Vertices.size();
            resource->VertexBuffers[i] = CreateBufferAllocation(
                vertexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                subMesh.Vertices.data(),
                *deviceModule);
        }

        if (!subMesh.Indices.empty())
        {
            const VkDeviceSize indexBufferSize = sizeof(subMesh.Indices[0]) * subMesh.Indices.size();
            resource->IndexBuffers[i] = CreateBufferAllocation(
                indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                subMesh.Indices.data(),
                *deviceModule);
        }

        if (!subMesh.AnimationVertexData.empty())
        {
            const VkDeviceSize animationBufferSize =
                sizeof(subMesh.AnimationVertexData[0]) * subMesh.AnimationVertexData.size();
            resource->AnimationBuffers[i] = CreateBufferAllocation(
                animationBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                subMesh.AnimationVertexData.data(),
                *deviceModule);
        }

        resource->Meshlets[i] = std::make_shared<Meshlet>();
        resource->Meshlets[i]->GenerateMeshlet(subMesh.Vertices, subMesh.Indices);
    }

    return resource;
}
