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

struct CrossFadeState
{
    bool active = false;
    float elapsed = 0.0f;
    float duration = 0.2f;

    std::shared_ptr<Animation> from = nullptr;
    std::shared_ptr<Animation> to = nullptr;

    float fromTime = 0.0f;  // ticks
    float toTime = 0.0f;    // ticks

    bool loopFrom = true;
    bool loopTo = true;
};

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
    bool m_loop;

    char* animationbuffer;
    Bone* auxiliarBone = nullptr;
    std::map<std::string, std::shared_ptr<ComputeNode>> computeNodes;
    CrossFadeState mFade;

private:
    static glm::mat4 ComposeTRS(const BoneTRS& trs);
    static void AdvanceTime(const Animation& anim, float& inOutTimeTicks, float dt, bool loop);
    void EvaluateLocalPoseTRS(Animation& anim, float timeTicks, std::vector<BoneTRS>& outPose);
    void BuildFinalFromLocalPose(const Animation& anim, const std::vector<BoneTRS>& localPose);
    
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

    void CrossFadeTo(std::shared_ptr<Animation> next, float durationSec, bool loopNext, bool loopFrom);
    bool IsInTransition() const { return mFade.active; }

    void UpdateAnimation(float dt, bool loop);
    void PlayAnimation(std::shared_ptr<Animation> pAnimation, bool loop);
    void CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform);
    std::shared_ptr<std::vector<glm::mat4>> GetFinalBoneMatrices();
    void CleanAnimatorUBO();

    // DEMO
    void ChangeAnimation(std::shared_ptr<Animation> newAnimation);
};

#endif
