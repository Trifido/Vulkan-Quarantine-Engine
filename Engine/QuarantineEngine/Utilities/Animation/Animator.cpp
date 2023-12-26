#include "Animator.h"
#include <SynchronizationModule.h>
#include <ShaderManager.h>

#include <chrono>

Animator::Animator()
{
    this->deviceModule = DeviceModule::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();

	m_CurrentTime = 0.0;
    m_FinalBoneMatrices = std::make_shared<std::vector<glm::mat4>>(std::vector<glm::mat4>(NUM_BONES, glm::mat4(1.0f)));
}

void Animator::InitializeComputeNodes(std::vector<std::string> idChilds)
{
    auto shaderManager = ShaderManager::getInstance();
    for (uint32_t i = 0; i < idChilds.size(); i++)
    {
        this->computeNodes[idChilds[i]] = std::make_shared<ComputeNode>(shaderManager->GetShader("default_skinning"));
        this->computeNodeManager->AddComputeNode("default_skinning", this->computeNodes[idChilds[i]]);
    }
}

void Animator::UpdateUBOAnimation()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();

    for (auto cn : computeNodes)
    {
        void* data;
        vkMapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame], 0, cn.second->computeDescriptor->uboSizes["UniformAnimation"], 0, &data);
        memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), cn.second->computeDescriptor->uboSizes["UniformAnimation"]);
        vkUnmapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame]);
    }
}

void Animator::SetVertexBufferInComputeNode(std::string id, VkBuffer vertexBuffer, uint32_t numElements)
{
    this->computeNodes[id]->NElements = numElements;
    this->computeNodes[id]->computeDescriptor->InitializeSSBOData();

    uint32_t numSSBOs = this->computeNodes[id]->computeDescriptor->ssboData.size();

    uint32_t bufferSize = sizeof(PBRAnimationVertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[0]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[0] = bufferSize;
    this->computeNodes[id]->FillComputeBuffer(vertexBuffer, bufferSize);

    bufferSize = sizeof(AnimationVertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[1]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[1] = bufferSize;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame], 0, this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->computeNodes[id]->NElements), this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"]);
        vkUnmapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame]);
    }
}

std::shared_ptr<ComputeNode> Animator::GetComputeNode(std::string id)
{
    return this->computeNodes[id];
}

void Animator::UpdateAnimation(float dt)
{
    m_DeltaTime = dt;
    if (m_CurrentAnimation != nullptr)
    {
        m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
        m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
        CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        this->UpdateUBOAnimation();
    }
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
    m_CurrentTime = 0.0f;
    m_CurrentAnimation = pAnimation;

    CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        for (auto cn : computeNodes)
        {
            void* data;
            vkMapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame], 0, cn.second->computeDescriptor->uboSizes["UniformAnimation"], 0, &data);
            memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), cn.second->computeDescriptor->uboSizes["UniformAnimation"]);
            vkUnmapMemory(deviceModule->device, cn.second->computeDescriptor->ubos["UniformAnimation"]->uniformBuffersMemory[currentFrame]);
        }
    }
}

void Animator::CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    auxiliarBone = m_CurrentAnimation->FindBone(nodeName);

    glm::mat4 nodeTransform = node->transformation;
    if (auxiliarBone)
    {
        auxiliarBone->Update(m_CurrentTime);
        nodeTransform = auxiliarBone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;
    if (m_CurrentAnimation->animationData.m_BoneInfoMap.find(nodeName) != m_CurrentAnimation->animationData.m_BoneInfoMap.end())
    {
        int index = m_CurrentAnimation->animationData.m_BoneInfoMap[nodeName].id;
        if (index < NUM_BONES)
        {
            glm::mat4 offset = m_CurrentAnimation->animationData.m_BoneInfoMap[nodeName].offset;
            m_FinalBoneMatrices->at(index) = globalTransformation * offset;
        }
    }

    for (int i = 0; i < node->childrenCount; i++)
        CalculateBoneTransform(&node->children[i], globalTransformation);
}

std::shared_ptr<std::vector<glm::mat4>> Animator::GetFinalBoneMatrices()
{
    return m_FinalBoneMatrices;
}

void Animator::ChangeAnimation(std::shared_ptr<Animation> newAnimation)
{
    this->m_CurrentTime = 0.0f;
    this->m_DeltaTime = 0.0f;
    this->m_CurrentAnimation = newAnimation;
}

void Animator::CleanAnimatorUBO()
{
    this->auxiliarBone = nullptr;

    m_FinalBoneMatrices.reset();
    m_FinalBoneMatrices = nullptr;

    m_CurrentAnimation.reset();
    m_CurrentAnimation = nullptr;
}
