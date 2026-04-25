#pragma once

#ifndef QE_GEOMETRY_RESOURCE_CACHE_H
#define QE_GEOMETRY_RESOURCE_CACHE_H

#include <Meshlet.h>
#include <QEMeshData.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

struct QEGeometryBufferAllocation
{
    VkBuffer Buffer = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
};

struct QEGeometrySharedResource
{
    QEMesh Mesh;
    std::vector<QEGeometryBufferAllocation> VertexBuffers;
    std::vector<QEGeometryBufferAllocation> IndexBuffers;
    std::vector<QEGeometryBufferAllocation> AnimationBuffers;
    std::vector<std::shared_ptr<Meshlet>> Meshlets;

    ~QEGeometrySharedResource();
};

class QEGeometryResourceCache
{
public:
    static std::shared_ptr<QEGeometrySharedResource> Acquire(
        const std::string& key,
        const std::function<QEMesh()>& buildMeshFn);

    static void CollectGarbage();

private:
    static std::shared_ptr<QEGeometrySharedResource> CreateResource(const QEMesh& mesh);

private:
    static std::unordered_map<std::string, std::weak_ptr<QEGeometrySharedResource>> cache;
    static std::mutex cacheMutex;
};

#endif
