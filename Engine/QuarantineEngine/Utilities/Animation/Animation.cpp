#include "Animation.h"

Animation::Animation(const AnimationData& animData)
{
    this->animationData = animData;
}

const std::map<std::string, BoneInfo>& Animation::GetBoneIDMap()
{
    return animationData.m_BoneInfoMap;
}

Bone* Animation::FindBone(const std::string& name)
{
    auto iter = std::find_if(animationData.m_Bones.begin(), animationData.m_Bones.end(),
        [&](Bone& bone)
        {
            return bone.GetBoneName() == name;
        }
    );
    if (iter == animationData.m_Bones.end()) return nullptr;
    else return &(*iter);
}
