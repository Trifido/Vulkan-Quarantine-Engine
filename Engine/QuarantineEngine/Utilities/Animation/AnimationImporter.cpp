#include "AnimationImporter.h"

AnimationData AnimationImporter::LoadMeshAnimated(std::string path)
{
    Assimp::Importer importer;

    scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return AnimationData();
    }

    auto animation = scene->mAnimations[0];
    m_Duration = animation->mDuration;
    m_TicksPerSecond = animation->mTicksPerSecond;

    ProcessNode(m_RootNode, scene->mRootNode);
    ReadMissingBones(animation);

    return AnimationData();
}

void AnimationImporter::ReadMissingBones(const aiAnimation* animation)
{
    int size = animation->mNumChannels;

    for (int i = 0; i < size; i++)
    {
        auto channel = animation->mChannels[i];
        std::string boneName = channel->mNodeName.data;

        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
        {
            m_BoneInfoMap[boneName].id = m_BoneCounter;
            m_BoneCounter++;
        }
        m_Bones.push_back(Bone(channel->mNodeName.data, m_BoneInfoMap[channel->mNodeName.data].id, channel));
    }
}

void AnimationImporter::ProcessNode(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ProcessNode(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}
