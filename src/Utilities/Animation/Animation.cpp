#include "Animation.h"

Animation::Animation(const AnimationData& animData)
{
    this->animationData = animData;
    this->name = animData.animationName;
}

Bone* Animation::FindBone(const std::string& name)
{
    auto it = this->animationData.m_Bones.find(name);

    if (it != this->animationData.m_Bones.end())
    {
        return &(it->second);
    }

    return nullptr;
}

const Bone* Animation::FindBone(const std::string& name) const
{
    auto it = this->animationData.m_Bones.find(name);
    if (it != this->animationData.m_Bones.end())
        return &(it->second);
    return nullptr;
}

float Animation::GetTicksPerSecond() const { return static_cast<float>(animationData.m_TicksPerSecond); }
float Animation::GetDuration() const { return static_cast<float>(animationData.m_Duration); }
const AnimationNode& Animation::GetRootNode() const { return animationData.animationNodeData; }
