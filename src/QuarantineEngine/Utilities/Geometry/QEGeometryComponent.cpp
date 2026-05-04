#include "QEGeometryComponent.h"
#include "BufferManageModule.h"
#include <Helpers/ScopedTimer.h>
#include <Helpers/QEMemoryTrack.h>
#include <filesystem>

DeviceModule* QEGeometryComponent::deviceModule_ptr;

QEMesh* QEGeometryComponent::GetMesh()
{
    if (!geometryResource || geometryResource->Mesh.MeshData.empty())
    {
        return nullptr;
    }

    return &geometryResource->Mesh;
}

size_t QEGeometryComponent::GetIndicesCount(uint32_t meshIndex) const
{
    if (!geometryResource || meshIndex >= geometryResource->Mesh.MeshData.size())
    {
        throw std::out_of_range("Mesh index out of range");
    }
    return geometryResource->Mesh.MeshData[meshIndex].Indices.size();
}

std::unique_ptr<IQEMeshGenerator> QEGeometryComponent::GetGenerator(std::string name, std::string filepath)
{
    if (filepath != "QECore")
    {
        if (filepath.empty())
        {
            return nullptr;
        }

        return std::make_unique<QEMeshGenerator>(filepath);
    }

    if (name == "CubePrimitive")
    {
        return std::make_unique<CubeGenerator>();
    }
    else if (name == "SpherePrimitive")
    {
        return std::make_unique<SphereGenerator>();
    }
    else if (name == "QuadPrimitive")
    {
        return std::make_unique<QuadGenerator>();
    }
    else if (name == "PointPrimitive")
    {
        return std::make_unique<PointGenerator>();
    }
    else if (name == "FloorPrimitive")
    {
        return std::make_unique<FloorGenerator>();
    }
    else if (name == "GridPrimitive")
    {
        return std::make_unique<GridGenerator>();
    }
    else if (name == "TrianglePrimitive")
    {
        return std::make_unique<TriangleGenerator>();
    }
    else if (name == "CapsulePrimitive")
    {
        return std::make_unique<CapsuleGenerator>();
    }
    else if (name == "ConePrimitive")
    {
        return std::make_unique<ConeGenerator>();
    }
    else if (name == "CylinderPrimitive")
    {
        return std::make_unique<CylinderGenerator>();
    }
    else if (name == "PyramidPrimitive")
    {
        return std::make_unique<PyramidGenerator>();
    }
    else if (name == "TorusPrimitive")
    {
        return std::make_unique<TorusGenerator>();
    }

    return nullptr;
}

void QEGeometryComponent::ReleaseResources()
{
    if (geometryResource)
    {
        geometryResource.reset();
    }

    if (ownsBuffersDirectly && deviceModule_ptr != nullptr)
    {
        for (int i = 0; i < indexBufferMemory.size(); i++)
        {
            if (indexBufferMemory[i] != VK_NULL_HANDLE)
            {
                QE_DESTROY_BUFFER(deviceModule_ptr->device, indexBuffer[i], "QEGeometryComponent::ReleaseResources");
                QE_FREE_MEMORY(deviceModule_ptr->device, indexBufferMemory[i], "QEGeometryComponent::ReleaseResources");
                indexBufferMemory[i] = VK_NULL_HANDLE;
            }
        }

        for (int i = 0; i < vertexBufferMemory.size(); i++)
        {
            if (vertexBufferMemory[i] != VK_NULL_HANDLE)
            {
                QE_DESTROY_BUFFER(deviceModule_ptr->device, vertexBuffer[i], "QEGeometryComponent::ReleaseResources");
                QE_FREE_MEMORY(deviceModule_ptr->device, vertexBufferMemory[i], "QEGeometryComponent::ReleaseResources");
                vertexBufferMemory[i] = VK_NULL_HANDLE;
            }
        }

        for (int i = 0; i < animationBufferMemory.size(); i++)
        {
            if (animationBufferMemory[i] != VK_NULL_HANDLE)
            {
                QE_DESTROY_BUFFER(deviceModule_ptr->device, animationBuffer[i], "QEGeometryComponent::ReleaseResources");
                QE_FREE_MEMORY(deviceModule_ptr->device, animationBufferMemory[i], "QEGeometryComponent::ReleaseResources");
                animationBufferMemory[i] = VK_NULL_HANDLE;
            }
        }
    }

    ownsBuffersDirectly = false;
    vertexBuffer.clear();
    indexBuffer.clear();
    animationBuffer.clear();
    vertexBufferMemory.clear();
    indexBufferMemory.clear();
    animationBufferMemory.clear();
    meshlets_ptr.clear();
}

void QEGeometryComponent::QEStart()
{
    if (_QEStarted)
    {
        return;
    }

    if (deviceModule_ptr == nullptr)
    {
        return;
    }

    if (generator == nullptr)
    {
        generator = GetGenerator(_name, _filepath);
    }

    if (generator == nullptr)
    {
        return;
    }

    BuildMesh();

    QEGameComponent::QEStart();
}

void QEGeometryComponent::QEInit()
{
    QEGameComponent::QEInit();
}

void QEGeometryComponent::QEUpdate()
{
}

void QEGeometryComponent::QEDestroy()
{
    if (_QEDestroyed)
    {
        return;
    }

    ReleaseResources();

    QEGameComponent::QEDestroy();
}

void QEGeometryComponent::BuildMesh()
{
    if (!generator)
    {
        return;
    }

    ReleaseResources();

    geometryResource = QEGeometryResourceCache::Acquire(
        BuildResourceKey(),
        [this]()
        {
            return generator->GenerateQEMesh();
        });

    if (!geometryResource)
    {
        return;
    }

    _name = geometryResource->Mesh.Name;
    _filepath = geometryResource->Mesh.FilePath;

    SyncResourceViews();
}

void QEGeometryComponent::CreateMeshlets()
{
    SyncResourceViews();
}

void QEGeometryComponent::CreateIndexBuffers()
{
    if (!geometryResource)
    {
        return;
    }

    SyncResourceViews();
}

void QEGeometryComponent::CreateVertexBuffers()
{
    if (!geometryResource)
    {
        return;
    }

    SyncResourceViews();
}

void QEGeometryComponent::CreateAnimationVertexBuffers()
{
    if (!geometryResource)
    {
        return;
    }

    SyncResourceViews();
}

void QEGeometryComponent::CreateGeometryBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, const void* dataArray, VkBuffer& buffer, VkDeviceMemory& memory)
{
    ownsBuffersDirectly = true;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, dataArray, (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    BufferManageModule::createBuffer(bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory, *deviceModule_ptr);
    BufferManageModule::copyBuffer(stagingBuffer, buffer, bufferSize, *deviceModule_ptr);

    QE_DESTROY_BUFFER(deviceModule_ptr->device, stagingBuffer, "QEGeometryComponent::CreateGeometryBuffer");
    QE_FREE_MEMORY(deviceModule_ptr->device, stagingBufferMemory, "QEGeometryComponent::CreateGeometryBuffer");
}

std::string QEGeometryComponent::BuildResourceKey() const
{
    namespace fs = std::filesystem;

    if (_filepath == "QECore")
    {
        return "QECore::" + _name;
    }

    if (_filepath.empty())
    {
        return {};
    }

    std::error_code ec;
    fs::path meshPath = fs::path(_filepath);
    fs::path canonicalPath = fs::weakly_canonical(meshPath, ec);
    if (ec)
    {
        canonicalPath = fs::absolute(meshPath, ec);
        if (ec)
        {
            return meshPath.lexically_normal().generic_string();
        }
    }

    return canonicalPath.lexically_normal().generic_string();
}

void QEGeometryComponent::SyncResourceViews()
{
    if (!geometryResource)
    {
        return;
    }

    const size_t subMeshCount = geometryResource->Mesh.MeshData.size();

    vertexBuffer.resize(subMeshCount, VK_NULL_HANDLE);
    indexBuffer.resize(subMeshCount, VK_NULL_HANDLE);
    animationBuffer.resize(subMeshCount, VK_NULL_HANDLE);
    vertexBufferMemory.resize(subMeshCount, VK_NULL_HANDLE);
    indexBufferMemory.resize(subMeshCount, VK_NULL_HANDLE);
    animationBufferMemory.resize(subMeshCount, VK_NULL_HANDLE);
    meshlets_ptr = geometryResource->Meshlets;

    for (size_t i = 0; i < subMeshCount; ++i)
    {
        vertexBuffer[i] = geometryResource->VertexBuffers[i].Buffer;
        vertexBufferMemory[i] = geometryResource->VertexBuffers[i].Memory;
        indexBuffer[i] = geometryResource->IndexBuffers[i].Buffer;
        indexBufferMemory[i] = geometryResource->IndexBuffers[i].Memory;
        animationBuffer[i] = geometryResource->AnimationBuffers[i].Buffer;
        animationBufferMemory[i] = geometryResource->AnimationBuffers[i].Memory;
    }
}
