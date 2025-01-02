#pragma once
#ifndef ATMOSPHERE_SYSTEM_H
#define ATMOSPHERE_SYSTEM_H

#include <DeviceModule.h>
#include <ShaderModule.h>
#include <QESingleton.h>

class AtmosphereSystem : public QESingleton<AtmosphereSystem>
{
private:
    friend class QESingleton<AtmosphereSystem>; // Permitir acceso al constructor

    DeviceModule* device_ptr;
    std::shared_ptr<ShaderModule> skybox_cubemap_shader;
    std::shared_ptr<GeometryComponent>  _Mesh = nullptr;
    glm::mat4 model = glm::mat4(1.0f);

public:
    AtmosphereSystem();
    ~AtmosphereSystem();

    void InitializeResources();
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void Cleanup();
    void CleanLastResources();
};

#endif // !ATMOSPHERE_SYSTEM_H


