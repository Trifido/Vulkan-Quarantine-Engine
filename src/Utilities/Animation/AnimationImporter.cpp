#include "AnimationImporter.h"
#include <Animation/Bone.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <SanitizerHelper.h>

std::vector<AnimationData> AnimationImporter::LoadAnimation(std::string animationFilepath, std::unordered_map<std::string, BoneInfo> m_BoneInfoMap)
{
    std::vector<AnimationData> result = {};
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(animationFilepath, aiProcess_Triangulate);

    if (!scene || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return result;
    }

    size_t numBones = m_BoneInfoMap.size();

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
            animationData.m_BoneInfoMap[boneName].id = (int)numBones;
            animationData.m_BoneInfoMap[boneName].offset = glm::mat4(1.0f);
            numBones++;
        }
        animationData.m_Bones[boneName] = Bone(channel->mNodeName.data, animationData.m_BoneInfoMap[channel->mNodeName.data].id, channel);
    }
}

void AnimationImporter::ProcessNode(AnimationNode& dest, const aiNode* src)
{
    assert(src);

    dest.name = src->mName.data;
    dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
    dest.childrenCount = src->mNumChildren;

    for (unsigned int i = 0; i < src->mNumChildren; i++)
    {
        AnimationNode newData;
        ProcessNode(newData, src->mChildren[i]);
        dest.children.push_back(newData);
    }
}

static aiNode* CloneNodeHierarchy(const aiNode* src)
{
    if (!src) return nullptr;
    aiNode* dst = new aiNode();

    // Copia nombre y transform
    dst->mName = src->mName;
    dst->mTransformation = src->mTransformation;

    // Sin mallas
    dst->mNumMeshes = 0;
    dst->mMeshes = nullptr;

    // Hijos
    dst->mNumChildren = src->mNumChildren;
    if (dst->mNumChildren > 0)
    {
        dst->mChildren = new aiNode * [dst->mNumChildren];
        for (unsigned i = 0; i < dst->mNumChildren; ++i) {
            dst->mChildren[i] = CloneNodeHierarchy(src->mChildren[i]);
            if (dst->mChildren[i]) dst->mChildren[i]->mParent = dst;
        }
    }
    else
    {
        dst->mChildren = nullptr;
    }
    return dst;
}

static aiNodeAnim* CloneNodeAnim(const aiNodeAnim* src)
{
    aiNodeAnim* dst = new aiNodeAnim();
    dst->mNodeName = src->mNodeName;
    dst->mPreState = src->mPreState;
    dst->mPostState = src->mPostState;

    // Position keys
    dst->mNumPositionKeys = src->mNumPositionKeys;
    if (dst->mNumPositionKeys)
    {
        dst->mPositionKeys = new aiVectorKey[dst->mNumPositionKeys];
        for (unsigned i = 0; i < dst->mNumPositionKeys; ++i)
        {
            dst->mPositionKeys[i].mTime = src->mPositionKeys[i].mTime;
            dst->mPositionKeys[i].mValue = src->mPositionKeys[i].mValue;
        }
    }
    else dst->mPositionKeys = nullptr;

    // Rotation keys
    dst->mNumRotationKeys = src->mNumRotationKeys;
    if (dst->mNumRotationKeys)
    {
        dst->mRotationKeys = new aiQuatKey[dst->mNumRotationKeys];
        for (unsigned i = 0; i < dst->mNumRotationKeys; ++i)
        {
            dst->mRotationKeys[i].mTime = src->mRotationKeys[i].mTime;
            dst->mRotationKeys[i].mValue = src->mRotationKeys[i].mValue;
        }
    }
    else dst->mRotationKeys = nullptr;

    // Scaling keys
    dst->mNumScalingKeys = src->mNumScalingKeys;
    if (dst->mNumScalingKeys)
    {
        dst->mScalingKeys = new aiVectorKey[dst->mNumScalingKeys];
        for (unsigned i = 0; i < dst->mNumScalingKeys; ++i)
        {
            dst->mScalingKeys[i].mTime = src->mScalingKeys[i].mTime;
            dst->mScalingKeys[i].mValue = src->mScalingKeys[i].mValue;
        }
    }
    else dst->mScalingKeys = nullptr;

    return dst;
}

static aiAnimation* CloneAnimation(const aiAnimation* src)
{
    aiAnimation* dst = new aiAnimation();
    // Copia metadatos básicos
    dst->mName = src->mName;
    dst->mDuration = src->mDuration;
    dst->mTicksPerSecond = src->mTicksPerSecond;

    // Canales de nodos
    dst->mNumChannels = src->mNumChannels;
    if (dst->mNumChannels)
    {
        dst->mChannels = new aiNodeAnim * [dst->mNumChannels];
        for (unsigned i = 0; i < dst->mNumChannels; ++i)
        {
            dst->mChannels[i] = CloneNodeAnim(src->mChannels[i]);
        }
    }
    else
    {
        dst->mChannels = nullptr;
    }

    // (Opcional) Mesh channels / morphs: si no usas, déjalo vacío
    dst->mNumMeshChannels = 0;
    dst->mMeshChannels = nullptr;
    dst->mNumMorphMeshChannels = 0;
    dst->mMorphMeshChannels = nullptr;

    return dst;
}

void AnimationImporter::DestroyScene(aiScene* scn)
{
    if (!scn) return;

    delete scn->mRootNode;
    scn->mRootNode = nullptr;

    // Animations
    for (unsigned i = 0; i < scn->mNumAnimations; ++i)
    {
        aiAnimation* a = scn->mAnimations[i];
        if (a)
        {
            delete a;
        }
    }
    delete[] scn->mAnimations;
    scn->mAnimations = nullptr;
    scn->mNumAnimations = 0;

    // Nada de materiales, mallas, cámaras, luces en estas escenas mínimas
    delete scn;
}

bool AnimationImporter::ImportAnimation(const std::string& inputPath, const std::string& outputDir)
{
    unsigned int flags = aiProcess_Triangulate;
    Assimp::Importer importer;
    const aiScene* srcScene = importer.ReadFile(inputPath, flags);

    if (!srcScene || !srcScene->mRootNode)
    {
        std::fprintf(stderr, "ASSIMP ERROR: %s\n", importer.GetErrorString());
        return false;
    }

    return AnimationImporter::ImportAnimation(srcScene, outputDir);
}

bool AnimationImporter::ImportAnimation(const aiScene* srcScene, const std::string& outputDir)
{
    if (srcScene->mNumAnimations == 0)
    {
        std::fprintf(stderr, "No se encontraron animaciones en el archivo.\n");
        return false;
    }

    // Asegura el directorio de salida
    Assimp::Exporter exporter;

    for (unsigned animIndex = 0; animIndex < srcScene->mNumAnimations; ++animIndex) {
        const aiAnimation* srcAnim = srcScene->mAnimations[animIndex];

        // Crea una escena mínima
        aiScene* outScene = new aiScene();

        // 1) Copia de jerarquía de nodos completa
        outScene->mRootNode = CloneNodeHierarchy(srcScene->mRootNode);

        // 2) Sin mallas, materiales, cámaras, luces
        outScene->mNumMeshes = 0;           outScene->mMeshes = nullptr;
        outScene->mNumMaterials = 0;        outScene->mMaterials = nullptr;
        outScene->mNumCameras = 0;          outScene->mCameras = nullptr;
        outScene->mNumLights = 0;           outScene->mLights = nullptr;
        outScene->mNumTextures = 0;         outScene->mTextures = nullptr;

        // 3) Una animación: copia profunda
        outScene->mNumAnimations = 1;
        outScene->mAnimations = new aiAnimation * [1];
        outScene->mAnimations[0] = CloneAnimation(srcAnim);

        SanitizerHelper::SanitizeSceneNames(outScene);

        // Nombre de archivo: usa el nombre de la animación si existe
        std::string animName = srcAnim->mName.length > 0
            ? std::string(srcAnim->mName.C_Str())
            : ("Anim_" + std::to_string(animIndex));

        // Sanitiza nombre (por si acaso)
        for (auto& ch : animName) {
            if (ch == '/' || ch == '\\' || ch == ':' || ch == '*'
                || ch == '?' || ch == '\"' || ch == '<' || ch == '>'
                || ch == '|')
                ch = '_';
        }

        fs::path outPath = fs::path(outputDir) / (animName + ".glb");

        // Exporta como GLB binario (más compacto y rápido de I/O)
        aiReturn ret = exporter.Export(outScene, "glb2", outPath.string());
        if (ret != aiReturn_SUCCESS)
        {
            std::fprintf(stderr, "Export ERROR (%s): %s\n",
                outPath.string().c_str(), exporter.GetErrorString());
            DestroyScene(outScene);
            return false;
        }

        DestroyScene(outScene);
        std::printf("Exportada animación %u -> %s\n", animIndex, outPath.string().c_str());
    }

    return true;
}


bool AnimationImporter::IsGlbFile(const fs::path& p)
{
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".glb";
}

std::vector<fs::path> AnimationImporter::ListGlbInDir(const fs::path& dir)
{
    std::vector<fs::path> out;
    std::error_code ec;

    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
        std::cerr << "[ListGlbInDir] No existe o no es directorio: " << dir << "\n";
        return out;
    }

    for (const auto& e : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec))
    {
        if (ec) { ec.clear(); continue; }
        if (!e.is_regular_file(ec)) { ec.clear(); continue; }
        if (AnimationImporter::IsGlbFile(e.path())) {
            out.push_back(e.path());
        }
    }

    std::sort(out.begin(), out.end());
    return out;
}
