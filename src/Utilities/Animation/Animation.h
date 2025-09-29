#pragma once
#ifndef ANIMATION_H
#define ANIMATION_H

#include <map>
#include <Bone.h>
#include <AnimationResources.h>

class Animation
{
public:
    AnimationData animationData;
    std::string name;

public:
    Animation(const AnimationData& animData);
    float GetTicksPerSecond();
    float GetDuration();
    const AnimationNode& GetRootNode();
    Bone* FindBone(const std::string& name);
};

#endif
