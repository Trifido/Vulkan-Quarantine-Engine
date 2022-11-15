#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <vector>
#include <map>
#include <Bone.h>
#include <Animation/AnimationImporter.h>

class Animation
{
private:
    float m_Duration;
    int m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, BoneInfo> m_BoneInfoMap;

public:
    inline float GetTicksPerSecond() { return m_TicksPerSecond; }
    inline float GetDuration() { return m_Duration; }
    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
    const std::map<std::string, BoneInfo>& GetBoneIDMap();
    Bone* FindBone(const std::string& name);
};

#endif
