#pragma once
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <Animation/Animation.h>
#include <AnimationResources.h>
#include <DescriptorBuffer.h>
#include <Compute/ComputeNodeManager.h>

class Animator
{
private:
    DeviceModule* deviceModule;
    ComputeNodeManager* computeNodeManager;
    const int NUM_BONES = 200;
    std::shared_ptr<std::vector<glm::mat4>> m_FinalBoneMatrices;
    std::shared_ptr<Animation> m_CurrentAnimation = nullptr;
    float m_CurrentTime;
    float m_DeltaTime;

    //VkDeviceMemory  computeBufferMemory1 = VK_NULL_HANDLE;
    //VkDeviceMemory  computeBufferMemory2 = VK_NULL_HANDLE;
    //VkBuffer computeBuffer1;
    //VkBuffer computeBuffer2;

    char* animationbuffer;
    Bone* auxiliarBone = nullptr;
    std::vector<std::shared_ptr<ComputeNode>> computeNodes;

public:
    std::shared_ptr<AnimationUniform> animationUniform_ptr;
    VkDeviceSize animationUniformSize = 0;
    std::shared_ptr<UniformBufferObject> animationUBO = nullptr;
    
public:
    Animator();
    void AssignAnimationBuffer(std::shared_ptr<DescriptorBuffer> descriptorBuffer);
    void InitializeComputeNodes(uint32_t numComputeNodes);
    void SetVertexBufferInComputeNode(uint32_t idx, VkBuffer vertexBuffer, uint32_t bufferSize, uint32_t numElements);
    std::shared_ptr<ComputeNode> GetComputeNode(uint32_t id);
    void InitializeUBOAnimation(std::shared_ptr<ShaderModule> shader_ptr);
    void UpdateUBOAnimation();
    void WriteToAnimationBuffer(char* data, size_t& position, const size_t& sizeToCopy);
    void UpdateAnimation(float dt);
    void PlayAnimation(std::shared_ptr<Animation> pAnimation);
    void CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform);
    std::shared_ptr<std::vector<glm::mat4>> GetFinalBoneMatrices();
    void CleanAnimatorUBO();

    // DEMO
    void ChangeAnimation(std::shared_ptr<Animation> newAnimation);
};

#endif
