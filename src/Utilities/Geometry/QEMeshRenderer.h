#pragma once

#ifndef QE_MESH_RENDERER_H
#define QE_MESH_RENDERER_H

#include "QEGameComponent.h"
#include <vulkan/vulkan.h>  
#include <memory>
#include <AnimationComponent.h>

class QEMeshRenderer : public QEGameComponent
{
private:
    DeviceModule* deviceModule = nullptr;
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;
    std::shared_ptr<AnimationComponent> animationComponent = nullptr;
    std::shared_ptr<QEGeometryComponent> geometryComponent = nullptr;
    std::vector<std::shared_ptr<QEMaterial>>& materialComponents;
    std::shared_ptr<Transform> transformComponent = nullptr;
public:
    bool IsMeshShaderPipeline = false;

public:
    QEMeshRenderer();
    ~QEMeshRenderer() {}

    void QEStart() override;
    void QEUpdate() override;
    void QERelease() override;

    void SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout);
};

#endif // !QE_MESH_RENDERER_H
