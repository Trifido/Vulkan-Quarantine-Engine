#include "Animator.h"
#include <SynchronizationModule.h>
#include <ShaderManager.h>

#include <chrono>
#include <MeshImporter.h>

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
    VkDeviceSize size = sizeof(glm::mat4) * 200;

    for (auto cn : computeNodes)
    {
        void* data = nullptr;
        // ssboData[3] = paleta huesos
        vkMapMemory(deviceModule->device,
            cn.second->computeDescriptor->ssboData[3]->uniformBuffersMemory[currentFrame],
            0, size, 0, &data);

        memcpy(data, m_FinalBoneMatrices->data(), (size_t)size);
        vkUnmapMemory(deviceModule->device,
            cn.second->computeDescriptor->ssboData[3]->uniformBuffersMemory[currentFrame]);
    }
}

void Animator::SetVertexBufferInComputeNode(std::string id, VkBuffer vertexBuffer, VkBuffer animationVertexBuffer, uint32_t numElements)
{
    this->computeNodes[id]->NElements = numElements;
    this->computeNodes[id]->computeDescriptor->InitializeSSBOData();

    uint32_t bufferSize = sizeof(Vertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[0]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[0] = bufferSize;
    this->computeNodes[id]->FillComputeBuffer(0, vertexBuffer, bufferSize);

    bufferSize = sizeof(AnimationVertexData) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[1]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[1] = bufferSize;
    this->computeNodes[id]->FillComputeBuffer(1, animationVertexBuffer, bufferSize);

    bufferSize = sizeof(Vertex) * numElements;
    this->computeNodes[id]->computeDescriptor->ssboData[2]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[2] = bufferSize;

    // Bones palette SSBO (binding 3)
    bufferSize = sizeof(glm::mat4) * 200;
    this->computeNodes[id]->computeDescriptor->ssboData[3]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeNodes[id]->computeDescriptor->ssboSize[3] = bufferSize;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        void* data;
        vkMapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame], 0, this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"], 0, &data);
        memcpy(data, static_cast<const void*>(&this->computeNodes[id]->NElements), this->computeNodes[id]->computeDescriptor->uboSizes["UniformVertexParam"]);
        vkUnmapMemory(deviceModule->device, this->computeNodes[id]->computeDescriptor->ubos["UniformVertexParam"]->uniformBuffersMemory[currentFrame]);
    }
}

void Animator::InitializeDescriptorsComputeNodes()
{
    for (auto computeNode : this->computeNodes)
    {
        computeNode.second->InitializeComputeNode();
    }
}

std::shared_ptr<ComputeNode> Animator::GetComputeNode(std::string id)
{
    return this->computeNodes[id];
}

float Animator::GetTimeTicks() const { return m_CurrentTime; }

float Animator::GetDurationTicks() const { return m_CurrentAnimation ? m_CurrentAnimation->GetDuration() : 0.0f; }

float Animator::GetTicksPerSecond() const { return m_CurrentAnimation ? std::max(m_CurrentAnimation->GetTicksPerSecond(), 1.0f) : 25.0f; }

float Animator::GetNormalizedTime() const
{
    float d = GetDurationTicks();
    return d > 0.0f ? m_CurrentTime / d : 0.0f;
}

void Animator::UpdateAnimation(float dt, bool loop)
{
    m_DeltaTime = dt;
    if (!m_CurrentAnimation) return;

    float tps = m_CurrentAnimation->GetTicksPerSecond();
    if (tps <= 0.0f) tps = 25.0f;

    m_CurrentTime += tps * dt;

    const float duration = m_CurrentAnimation->GetDuration();

    if (loop)
    {
        if (duration > 0.0f)
        {
            m_CurrentTime = std::fmod(m_CurrentTime, duration);
            if (m_CurrentTime < 0.0f) m_CurrentTime += duration;
        }
        else
        {
            m_CurrentTime = 0.0f;
        }
    }
    else if (m_CurrentTime >= duration)
    {
        m_CurrentTime = std::max(0.0f, std::nextafter(duration, 0.0f));
    }

    auto rootNode = m_CurrentAnimation->GetRootNode();
    CalculateBoneTransform(&rootNode, glm::mat4(1.0f));
    this->UpdateUBOAnimation();
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation)
{
    if (pAnimation == m_CurrentAnimation)
        return;

    m_CurrentTime = 0.0f;
    m_CurrentAnimation = pAnimation;

    auto rootNode = m_CurrentAnimation->GetRootNode();
    CalculateBoneTransform(&rootNode, glm::mat4(1.0f));

    // Ahora la paleta va a SSBO (binding 3) en lugar de UBO
    VkDeviceSize bonesSize = sizeof(glm::mat4) * NUM_BONES;

    for (int currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; currentFrame++)
    {
        for (auto& cn : computeNodes)
        {
            void* data = nullptr;
            vkMapMemory(deviceModule->device, cn.second->computeDescriptor->ssboData[3]->uniformBuffersMemory[currentFrame],  0, bonesSize, 0, &data);
            memcpy(data, static_cast<const void*>(m_FinalBoneMatrices->data()), (size_t)bonesSize);
            vkUnmapMemory(deviceModule->device,cn.second->computeDescriptor->ssboData[3]->uniformBuffersMemory[currentFrame]);
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
