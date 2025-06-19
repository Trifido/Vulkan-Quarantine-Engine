#pragma once

#ifndef QE_MESH_RENDERER_H
#define QE_MESH_RENDERER_H

#include "QEGameComponent.h"
#include <vulkan/vulkan.h>  
#include <memory>
#include <AnimationComponent.h>

class QEMeshRenderer : public QEGameComponent
{
    REFLECTABLE_DERIVED_COMPONENT(QEMeshRenderer, QEGameComponent)
private:
    DeviceModule* deviceModule = nullptr;
    PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;
    std::shared_ptr<AnimationComponent> animationComponent = nullptr;
    std::shared_ptr<QEGeometryComponent> geometryComponent = nullptr;
    std::vector<std::shared_ptr<QEMaterial>>& materialComponents;
    std::shared_ptr<Transform> transformComponent = nullptr;

public:
    REFLECT_PROPERTY(bool, IsMeshShaderPipeline)

public:
    QEMeshRenderer();
    ~QEMeshRenderer() {}

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;

    void SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout);
};

#endif // !QE_MESH_RENDERER_H
