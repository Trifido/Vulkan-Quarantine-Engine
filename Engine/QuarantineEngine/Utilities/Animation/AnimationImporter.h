#pragma once
#ifndef ANIMATION_IMPORTER_H
#define ANIMATION_IMPORTER_H

#include <assimp/scene.h>
#include <map>
#include <AnimationResources.h>

class AnimationImporter
{
private:
    static void ProcessNode(AnimationNode& dest, const aiNode* src);
    static void ReadMissingBones(const aiAnimation* animation, AnimationData& animationData, size_t numBones);

public:
    static std::vector<AnimationData> ImportAnimation(std::string path, std::unordered_map<std::string, BoneInfo> m_BoneInfoMap, size_t numBones);
};

#endif
