#include "Animation.h"

const std::map<std::string, BoneInfo>& Animation::GetBoneIDMap()
{
    return m_BoneInfoMap;
}

Bone* Animation::FindBone(const std::string& name)
{
    auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
        [&](Bone& bone)
        {
            return bone.GetBoneName() == name;
        }
    );
    if (iter == m_Bones.end()) return nullptr;
    else return &(*iter);
}
