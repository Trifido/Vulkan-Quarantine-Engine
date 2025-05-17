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
