#include "QEGeometryComponent.h"
#include "BufferManageModule.h"
#include <Helpers/ScopedTimer.h>
#include <Helpers/QEMemoryTrack.h>

DeviceModule* QEGeometryComponent::deviceModule_ptr;

QEMesh* QEGeometryComponent::GetMesh()
{
    if (mesh.MeshData.empty())
    {
        return nullptr;
    }

    return &mesh;
}

size_t QEGeometryComponent::GetIndicesCount(uint32_t meshIndex) const
{
    if (meshIndex < 0 || meshIndex >= mesh.MeshData.size())
    {
        throw std::out_of_range("Mesh index out of range");
    }
    return mesh.MeshData[meshIndex].Indices.size();
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
    for(int i = 0; i < indexBufferMemory.size(); i++)
    {
        if (indexBufferMemory[i] != VK_NULL_HANDLE)
        {
            QE_DESTROY_BUFFER(deviceModule_ptr->device, indexBuffer[i], "QEGeometryComponent::ReleaseResources");
            QE_FREE_MEMORY(deviceModule_ptr->device, indexBufferMemory[i], "QEGeometryComponent::ReleaseResources");
            indexBufferMemory[i] = VK_NULL_HANDLE;
        }
    }

    for(int i = 0; i < vertexBufferMemory.size(); i++)
    {
        if (vertexBufferMemory[i] != VK_NULL_HANDLE)
        {
            QE_DESTROY_BUFFER(deviceModule_ptr->device, vertexBuffer[i], "QEGeometryComponent::ReleaseResources");
            QE_FREE_MEMORY(deviceModule_ptr->device, vertexBufferMemory[i], "QEGeometryComponent::ReleaseResources");
            vertexBufferMemory[i] = VK_NULL_HANDLE;
        }
    }

    for(int i = 0; i < animationBufferMemory.size(); i++)
    {
        if (animationBufferMemory[i] != VK_NULL_HANDLE)
        {
            QE_DESTROY_BUFFER(deviceModule_ptr->device, animationBuffer[i], "QEGeometryComponent::ReleaseResources");
            QE_FREE_MEMORY(deviceModule_ptr->device, animationBufferMemory[i], "QEGeometryComponent::ReleaseResources");
            animationBufferMemory[i] = VK_NULL_HANDLE;
        }
    }
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

    mesh = generator->GenerateQEMesh();

    _name = mesh.Name;
    _filepath = mesh.FilePath;

    CreateIndexBuffers();
    CreateVertexBuffers();
    CreateAnimationVertexBuffers();
    CreateMeshlets();
}

void QEGeometryComponent::CreateMeshlets()
{
    meshlets_ptr.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        meshlets_ptr[i] = std::make_shared<Meshlet>();
        meshlets_ptr[i]->GenerateMeshlet(mesh.MeshData[i].Vertices, mesh.MeshData[i].Indices);
    }
}

void QEGeometryComponent::CreateIndexBuffers()
{
    indexBuffer.resize(mesh.MeshData.size());
    indexBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].Indices[0]) * mesh.MeshData[i].Indices.size();

        if (bufferSize == 0)
        {
            return;
        }

        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].Indices.data(), indexBuffer[i], indexBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateVertexBuffers()
{
    vertexBuffer.resize(mesh.MeshData.size());
    vertexBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].Vertices[0]) * mesh.MeshData[i].Vertices.size();
        if (bufferSize == 0)
        {
            return;
        }
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].Vertices.data(), vertexBuffer[i], vertexBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateAnimationVertexBuffers()
{
    animationBuffer.resize(mesh.MeshData.size());
    animationBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].AnimationVertexData[0]) * mesh.MeshData[i].AnimationVertexData.size();
        if (bufferSize == 0)
        {
            return;
        }
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].AnimationVertexData.data(), animationBuffer[i], animationBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateGeometryBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, const void* dataArray, VkBuffer& buffer, VkDeviceMemory& memory)
{
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
