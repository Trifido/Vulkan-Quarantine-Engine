#pragma once
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <Animation/Animation.h>
#include <QEAnimationResources.h>
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

    char* animationbuffer;
    Bone* auxiliarBone = nullptr;
    std::map<std::string, std::shared_ptr<ComputeNode>> computeNodes;
    
public:
    Animator();
    void InitializeComputeNodes(std::vector<std::string> idChilds);
    void SetVertexBufferInComputeNode(std::string id, VkBuffer vertexBuffer, VkBuffer animationVertexBuffer, uint32_t numElements);
    void InitializeDescriptorsComputeNodes();
    std::shared_ptr<ComputeNode> GetComputeNode(std::string id);
    void UpdateUBOAnimation();

    float GetTimeTicks() const;
    float GetDurationTicks() const;
    float GetTicksPerSecond() const;
    float GetNormalizedTime() const;

    void UpdateAnimation(float dt, bool loop);
    void PlayAnimation(std::shared_ptr<Animation> pAnimation);
    void CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform);
    std::shared_ptr<std::vector<glm::mat4>> GetFinalBoneMatrices();
    void CleanAnimatorUBO();

    // DEMO
    void ChangeAnimation(std::shared_ptr<Animation> newAnimation);
};

#endif
