#pragma once
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <Animation/Animation.h>
#include <AnimationResources.h>
#include <DescriptorModule.h>

class Animator
{
private:
    const int NUM_BONES = 200;
    std::vector<std::shared_ptr<DescriptorModule>> descriptors;
    std::shared_ptr<std::vector<glm::mat4>> m_FinalBoneMatrices;
    std::shared_ptr<Animation> m_CurrentAnimation = nullptr;
    float m_CurrentTime;
    float m_DeltaTime;

    VkDeviceMemory  computeBufferMemory1 = VK_NULL_HANDLE;
    VkDeviceMemory  computeBufferMemory2 = VK_NULL_HANDLE;
    VkBuffer computeBuffer1;
    VkBuffer computeBuffer2;

public:
    std::shared_ptr<AnimationUniform> animationUniform_ptr;

private:
    void CreateShaderStorageBuffers();
public:
    Animator();
    void AddDescriptor(std::shared_ptr<DescriptorModule> descriptor);
    void UpdateAnimation(float dt);
    void PlayAnimation(std::shared_ptr<Animation> pAnimation);
    void CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform);
    std::shared_ptr<std::vector<glm::mat4>> GetFinalBoneMatrices();

    // DEMO
    void ChangeAnimation(std::shared_ptr<Animation> newAnimation);
};

#endif
