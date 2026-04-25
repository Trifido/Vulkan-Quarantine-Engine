#pragma once

#ifndef QE_GEOMETRY_COMPONENT_H
#define QE_GEOMETRY_COMPONENT_H

#include "QEGameComponent.h"
#include <Geometry/Vertex.h>
#include <DeviceModule.h>
#include <QEMeshGenerator.h>
#include <Meshlet.h>
#include <memory> 
#include <QEGeometryResourceCache.h>

class QEGeometryComponent : public QEGameComponent
{
public:
    REFLECTABLE_DERIVED_COMPONENT(QEGeometryComponent, QEGameComponent)

protected:
    REFLECT_PROPERTY(std::string, _name)
    REFLECT_PROPERTY(std::string, _filepath)
    std::vector<VkDeviceMemory> vertexBufferMemory = { VK_NULL_HANDLE };
    std::vector<VkDeviceMemory> indexBufferMemory = { VK_NULL_HANDLE };
    std::vector<VkDeviceMemory> animationBufferMemory = { VK_NULL_HANDLE };
    bool ownsBuffersDirectly = false;

    virtual void CreateIndexBuffers();
    virtual void CreateVertexBuffers();
    void CreateAnimationVertexBuffers();
    void CreateGeometryBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, const void* dataArray, VkBuffer& buffer, VkDeviceMemory& memory);

private:
    std::unique_ptr<IQEMeshGenerator> generator;
    std::shared_ptr<QEGeometrySharedResource> geometryResource;

public:
    QEGeometryComponent() = default;

    QEGeometryComponent(std::unique_ptr<IQEMeshGenerator> g)
        : generator(std::move(g)) {
    }

    static DeviceModule* deviceModule_ptr;
    std::vector<VkBuffer> vertexBuffer = { VK_NULL_HANDLE };
    std::vector<VkBuffer> indexBuffer = { VK_NULL_HANDLE };
    std::vector<VkBuffer> animationBuffer = { VK_NULL_HANDLE };
    std::vector<std::shared_ptr<Meshlet>> meshlets_ptr = {};
    std::string Path;

    void QEStart() override;

    void QEInit() override;

    void QEUpdate() override;

    void QEDestroy() override;

    void BuildMesh();

    QEMesh* GetMesh();

    size_t GetIndicesCount(uint32_t meshIndex) const;

    static std::unique_ptr<IQEMeshGenerator> GetGenerator(std::string name, std::string filepath);

private:
    void CreateMeshlets();
    void ReleaseResources();
    std::string BuildResourceKey() const;
    void SyncResourceViews();
};

#endif
