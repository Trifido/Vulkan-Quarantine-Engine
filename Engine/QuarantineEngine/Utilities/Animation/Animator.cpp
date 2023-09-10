#include "Animator.h"
#include <SynchronizationModule.h>
#include <ShaderManager.h>

Animator::Animator()
{
    this->deviceModule = DeviceModule::getInstance();
    this->computeNodeManager = ComputeNodeManager::getInstance();

	m_CurrentTime = 0.0;
    m_FinalBoneMatrices = std::make_shared<std::vector<glm::mat4>>(std::vector<glm::mat4>(NUM_BONES, glm::mat4(1.0f)));
    this->animationUniform_ptr = std::make_shared<AnimationUniform>();

    //this->computeNode->FillComputeBuffer(this->numParticles, sizeof(PBRVertex), particles.data());
}

void Animator::AssignAnimationBuffer(std::shared_ptr<DescriptorBuffer> descriptorBuffer)
{
    descriptorBuffer->animationUBO = this->animationUBO;
    descriptorBuffer->animationUniformSize = this->animationUniformSize;
}

void Animator::InitializeUBOAnimation(std::shared_ptr<ShaderModule> shader_ptr)
{
    auto reflect = shader_ptr->reflectShader;

    if (reflect.isUboAnimation)
    {
        this->animationUBO = std::make_shared<UniformBufferObject>();

        size_t position = 0;
        this->animationbuffer = new char[reflect.animationBufferSize];

        for each (auto name in reflect.animationUBOComponents)
        {
            if (name == "finalBonesMatrices")
            {
                this->WriteToAnimationBuffer(reinterpret_cast<char*>(&this->animationUniform_ptr->finalBonesMatrices), position, reflect.animationBufferSize);
            }
        }

        this->animationUBO->CreateUniformBuffer(reflect.animationBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
        this->UpdateUBOAnimation();
    }
}

void Animator::InitializeComputeNodes(uint32_t numComputeNodes)
{
    auto shaderManager = ShaderManager::getInstance();
    for (uint32_t i = 0; i < numComputeNodes; i++)
    {
        this->computeNodes.push_back(std::make_shared<ComputeNode>(shaderManager->GetShader("default_skinning")));
        this->computeNodeManager->AddComputeNode("default_skinning", this->computeNodes.back());
    }
}

void Animator::UpdateUBOAnimation()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    void* data;
    vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame], 0, this->animationUniformSize, 0, &data);
    memcpy(data, static_cast<const void*>(this->m_FinalBoneMatrices.get()->data()), this->animationUniformSize);
    vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame]);
}

void Animator::SetVertexBufferInComputeNode(uint32_t idx, VkBuffer vertexBuffer, uint32_t bufferSize, uint32_t numElements)
{
    if (idx < this->computeNodes.size())
    {
        this->computeNodes.at(idx)->NElements = numElements;
        this->computeNodes.at(idx)->computeDescriptor->InitializeSSBOData();

        uint32_t numSSBOs = this->computeNodes.at(idx)->computeDescriptor->ssboData.size();
        for (uint32_t i = 0; i < numSSBOs; i++)
        {
            this->computeNodes.at(idx)->computeDescriptor->ssboData[i]->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
            this->computeNodes.at(idx)->computeDescriptor->ssboSize[i] = bufferSize;
        }

        this->computeNodes.at(idx)->FillComputeBuffer(vertexBuffer, bufferSize);
    }
}

void Animator::WriteToAnimationBuffer(char* data, size_t& position, const size_t& sizeToCopy)
{
    memcpy(&animationbuffer[position], data, sizeToCopy);
    position += sizeToCopy;
    this->animationUniformSize += sizeToCopy;
}

std::shared_ptr<ComputeNode> Animator::GetComputeNode(uint32_t id)
{
    return this->computeNodes.at(id);
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
    m_CurrentAnimation = pAnimation;
    m_CurrentTime = 0.0f;
}

void Animator::CalculateBoneTransform(const AnimationNode* node, glm::mat4 parentTransform)
{
    std::string nodeName = node->name;
    auxiliarBone = m_CurrentAnimation->FindBone(nodeName);                                    // Cuidado con este puntero, no se libera

    glm::mat4 nodeTransform = node->transformation;

    if (auxiliarBone)
    {
        auxiliarBone->Update(m_CurrentTime);
        nodeTransform = auxiliarBone->GetLocalTransform();
    }

    glm::mat4 globalTransformation = parentTransform * nodeTransform;

    auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
    if (boneInfoMap.find(nodeName) != boneInfoMap.end())
    {
        int index = boneInfoMap[nodeName].id;
        if (index < NUM_BONES)
        {
            glm::mat4 offset = boneInfoMap[nodeName].offset;
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
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Material UBO
        if (this->animationUBO != nullptr)
        {
            vkDestroyBuffer(deviceModule->device, this->animationUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[i], nullptr);
        }
    }

    this->auxiliarBone = nullptr;

    this->animationUBO.reset();
    this->animationUBO = nullptr;

    delete [] this->animationbuffer;
    this->animationbuffer = nullptr;

    m_FinalBoneMatrices.reset();
    m_FinalBoneMatrices = nullptr;

    m_CurrentAnimation.reset();
    m_CurrentAnimation = nullptr;
}
