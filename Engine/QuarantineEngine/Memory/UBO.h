#pragma once
#ifndef UBO_H
#define UBO_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/matrix.hpp>
#include <DeviceModule.h>

struct LightUniform
{
    glm::vec4 position;
    glm::vec3 diffuse;
    glm::vec3 specular;
    //glm::vec3 ambient;
    //float cutOff;
    //float outerCutOff;
    float constant;
    float linear;
    float quadratic;
};

struct LightManagerUniform
{
    int numLights;
    alignas(16) LightUniform lights[8];
};

struct CameraUniform
{
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewproj;
    glm::vec3 position;
};

struct MaterialUniform
{
    //float heightScale;
    //float min_uv;
    //float max_uv;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
    //int idxBump;
    float shininess;
};

struct TransformUniform
{
    glm::mat4 model;
};

class UniformBufferObject
{
public:
    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VkDeviceMemory>     uniformBuffersMemory;

    void CreateUniformBuffer(VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device);
};

#endif
