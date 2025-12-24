#pragma once
#ifndef ANIMATION_IMPORTER_H
#define ANIMATION_IMPORTER_H

#include <assimp/scene.h>
#include <map>
#include <QEAnimationResources.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class AnimationImporter
{
private:
    static void ProcessNode(AnimationNode& dest, const aiNode* src);
    static void ReadMissingBones(const aiAnimation* animation, AnimationData& animationData, size_t numBones);
    static bool IsGlbFile(const fs::path& p);

public:
    static std::vector<AnimationData> LoadAnimation(std::string animationFilepath, std::unordered_map<std::string, BoneInfo> m_BoneInfoMap);
    static bool ImportAnimation(const std::string& inputPath, const std::string& outputDir);
    static bool ImportAnimation(const aiScene* srcScene, const std::string& outputDir);
    static std::vector<fs::path> ListGlbInDir(const fs::path& dir);
    static void DestroyScene(aiScene* scn);
};

#endif
