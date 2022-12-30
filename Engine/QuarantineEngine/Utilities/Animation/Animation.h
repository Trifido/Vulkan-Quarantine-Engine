#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <map>
#include <Bone.h>
#include <AnimationResources.h>

class Animation
{
private:
    AnimationData animationData;

public:
    std::string name;

public:
    Animation(const AnimationData& animData);
    inline float GetTicksPerSecond() { return animationData.m_TicksPerSecond; }
    inline float GetDuration() { return animationData.m_Duration; }
    inline const AnimationNode& GetRootNode() { return animationData.animationNodeData; }
    const std::map<std::string, BoneInfo>& GetBoneIDMap();
    Bone* FindBone(const std::string& name);
};

#endif
