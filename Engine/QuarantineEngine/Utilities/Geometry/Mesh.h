#pragma once
#ifndef MESH_H
#define MESH_H

#include <vulkan/vulkan.hpp>
#include <string>
#include "GeometryComponent.h"

class Mesh : public GeometryComponent
{
private:
    std::string PATH;

public:
    Mesh();
    Mesh(std::string pathfile);
    void InitializeMesh() override;
    VkVertexInputBindingDescription getBindingDescription() override;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override;
};

#endif // !MESH_H
