#pragma once
#ifndef SKELETAL_COMPONENT_H
#define SKELETAL_COMPONENT_H

#include <GameComponent.h>
#include <AnimationResources.h>

class SkeletalComponent : GameComponent
{
public:
    size_t numBones;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
};

#endif
