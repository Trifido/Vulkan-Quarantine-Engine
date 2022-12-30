#include "AnimationImporter.h"
#include <Animation/Bone.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

std::vector<AnimationData> AnimationImporter::ImportAnimation(std::string path, std::map<std::string, BoneInfo> m_BoneInfoMap, size_t numBones)
{
    std::vector<AnimationData> result = {};
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return result;
    }

    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        AnimationData data = {};
        auto animation = scene->mAnimations[i];
        data.animationName = animation->mName.C_Str();
        data.m_Duration = animation->mDuration;
        data.m_TicksPerSecond = animation->mTicksPerSecond;
        data.m_BoneInfoMap = m_BoneInfoMap;

        AnimationNode m_RootNode;
        ProcessNode(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, data, numBones);

        data.animationNodeData = m_RootNode;

        result.push_back(data);
    }

    return result;
}

void AnimationImporter::ReadMissingBones(const aiAnimation* animation, AnimationData& animationData, size_t numBones)
{
    int size = animation->mNumChannels;

    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (animationData.m_BoneInfoMap.find(boneName) == animationData.m_BoneInfoMap.end())
        {
            animationData.m_BoneInfoMap[boneName].id = numBones;
            animationData.m_BoneInfoMap[boneName].offset = glm::mat4(1.0f);
            numBones++;
        }
        animationData.m_Bones.push_back(Bone(channel->mNodeName.data, animationData.m_BoneInfoMap[channel->mNodeName.data].id, channel));
    }
}

void AnimationImporter::ProcessNode(AnimationNode& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++)
    {
        AnimationNode newData;
        ProcessNode(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}
