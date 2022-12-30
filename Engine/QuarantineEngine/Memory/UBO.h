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
    float constant;
    glm::vec3 specular;
    float linear;
    glm::vec3 spotDirection;
    float quadratic;
    float spotCutOff;
    float spotExponent;
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
    float shininess;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
};

struct AnimationUniform
{
    alignas(16) glm::mat4 finalBonesMatrices[200];
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
