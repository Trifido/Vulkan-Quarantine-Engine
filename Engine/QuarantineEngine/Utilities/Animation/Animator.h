#pragma once
#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <vector>
#include <glm/glm.hpp>
#include <Animation/Animation.h>
#include <Animation/AnimationImporter.h>

class Animator
{
private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;

public:
    Animator(Animation* currentAnimation);
    void UpdateAnimation(float dt);
    void PlayAnimation(Animation* pAnimation);
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform);
    std::vector<glm::mat4> GetFinalBoneMatrices();
};

#endif
