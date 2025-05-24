#pragma once

#ifndef QE_GEOMETRY_COMPONENT_H
#define QE_GEOMETRY_COMPONENT_H

#include "QEGameComponent.h"
#include <Geometry/Vertex.h>
#include <DeviceModule.h>
#include <QEMeshGenerator.h>
#include <Meshlet.h>
#include <memory> 

class QEGeometryComponent : public QEGameComponent
{
private:
    std::unique_ptr<IQEMeshGenerator> generator;
    QEMesh mesh;

protected:
    std::vector<VkDeviceMemory> vertexBufferMemory = { VK_NULL_HANDLE };
    std::vector<VkDeviceMemory> indexBufferMemory = { VK_NULL_HANDLE };
    std::vector<VkDeviceMemory> animationBufferMemory = { VK_NULL_HANDLE };

    virtual void CreateIndexBuffers();
    virtual void CreateVertexBuffers();
    void CreateAnimationVertexBuffers();
    void CreateGeometryBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, const void* dataArray, VkBuffer& buffer, VkDeviceMemory& memory);

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

    virtual void cleanup();

    void QEStart() override;

    void QEUpdate() override;

    void QERelease() override;

    void BuildMesh();

    const QEMesh* GetMesh() const;

    size_t GetIndicesCount(uint32_t meshIndex) const;

private:
    void CreateMeshlets();
};

#endif
