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

    if (!mFade.active)
    {
        AdvanceTime(*m_CurrentAnimation, m_CurrentTime, dt, loop);

        auto rootNode = m_CurrentAnimation->GetRootNode();
        CalculateBoneTransform(&rootNode, glm::mat4(1.0f));
        UpdateUBOAnimation();
        return;
    }

    // Cross-fade
    mFade.elapsed += dt;
    float alpha = glm::clamp(mFade.elapsed / mFade.duration, 0.0f, 1.0f);

    AdvanceTime(*mFade.from, mFade.fromTime, dt, mFade.loopFrom);
    AdvanceTime(*mFade.to, mFade.toTime, dt, mFade.loopTo);

    std::vector<BoneTRS> A, B, M;
    EvaluateLocalPoseTRS(*mFade.from, mFade.fromTime, A);
    EvaluateLocalPoseTRS(*mFade.to, mFade.toTime, B);

    M.resize(NUM_BONES);

    for (int i = 0; i < NUM_BONES; ++i)
    {
        BoneTRS a = A[i];
        BoneTRS b = B[i];

        if (!a.valid && b.valid) a = b;
        if (!b.valid && a.valid) b = a;

        M[i].t = glm::mix(a.t, b.t, alpha);
        M[i].s = glm::mix(a.s, b.s, alpha);

        glm::quat qa = a.r;
        glm::quat qb = b.r;
        if (glm::dot(qa, qb) < 0.0f) qb = -qb;
        M[i].r = glm::normalize(glm::slerp(qa, qb, alpha));

        M[i].valid = a.valid || b.valid;
    }

    // Construyes paleta (usa skeleton del "to" como referencia)
    BuildFinalFromLocalPose(*mFade.to, M);
    UpdateUBOAnimation();

    if (alpha >= 1.0f)
    {
        m_CurrentAnimation = mFade.to;
        m_CurrentTime = mFade.toTime;
        m_loop = mFade.loopTo;
        mFade = CrossFadeState{};
    }
}

void Animator::PlayAnimation(std::shared_ptr<Animation> pAnimation, bool loop)
{
    if (pAnimation == m_CurrentAnimation)
        return;

    m_CurrentTime = 0.0f;
    m_CurrentAnimation = pAnimation;
    m_loop = loop;

    auto rootNode = m_CurrentAnimation->GetRootNode();
    CalculateBoneTransform(&rootNode, glm::mat4(1.0f));

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


glm::mat4 Animator::ComposeTRS(const BoneTRS& trs)
{
    return glm::translate(glm::mat4(1.0f), trs.t)
        * glm::toMat4(trs.r)
        * glm::scale(glm::mat4(1.0f), trs.s);
}

void Animator::AdvanceTime(const Animation& anim, float& t, float dt, bool loop)
{
    float tps = anim.GetTicksPerSecond();
    if (tps <= 0.0f) tps = 25.0f;

    t += tps * dt;

    float duration = anim.GetDuration();
    if (duration <= 0.0f) { t = 0.0f; return; }

    if (loop)
    {
        t = std::fmod(t, duration);
        if (t < 0.0f) t += duration;
    }
    else
    {
        if (t >= duration)
            t = std::max(0.0f, std::nextafter(duration, 0.0f));
    }
}

void Animator::CrossFadeTo(std::shared_ptr<Animation> next, float durationSec, bool loopNext, bool loopFrom)
{
    if (!next) return;

    // Si no hay current anim, cae a PlayAnimation
    if (!m_CurrentAnimation)
    {
        PlayAnimation(next, loopNext);
        return;
    }

    // Si ya es la misma, no haces nada
    if (next == m_CurrentAnimation && !mFade.active)
        return;

    mFade.active = true;
    mFade.elapsed = 0.0f;
    mFade.duration = std::max(0.001f, durationSec);

    mFade.from = m_CurrentAnimation;
    mFade.to = next;

    mFade.fromTime = m_CurrentTime;
    mFade.toTime = 0.0f;      // para idle->walk suele ser lo correcto

    mFade.loopFrom = loopFrom;
    mFade.loopTo = loopNext;
}

void Animator::EvaluateLocalPoseTRS(Animation& anim, float timeTicks, std::vector<BoneTRS>& outPose)
{
    outPose.assign(NUM_BONES, BoneTRS{});

    std::function<void(const AnimationNode*)> dfs = [&](const AnimationNode* node)
        {
            const std::string& nodeName = node->name;

            // Solo nos interesan nombres que existan en BoneInfoMap (tienen ID)
            auto itInfo = anim.animationData.m_BoneInfoMap.find(nodeName);
            if (itInfo != anim.animationData.m_BoneInfoMap.end())
            {
                const int idx = itInfo->second.id;
                if (idx >= 0 && idx < NUM_BONES)
                {
                    if (const Bone* b = anim.FindBone(nodeName))
                    {
                        // SampleTRS() debe ser const (no mutar Bone)
                        BoneTRS trs = b->SampleTRS(timeTicks);
                        trs.valid = true;
                        outPose[idx] = trs;
                    }
                    // Si no hay canal animado para ese hueso:
                    // outPose[idx] se queda identity; la bind pose la obtendremos desde node->transformation al reconstruir global.
                }
            }

            for (int i = 0; i < node->childrenCount; ++i)
                dfs(&node->children[i]);
        };

    const AnimationNode& root = anim.GetRootNode();
    dfs(&root);
}

void Animator::BuildFinalFromLocalPose(const Animation& anim, const std::vector<BoneTRS>& localPose)
{
    std::function<void(const AnimationNode*, const glm::mat4&)> dfs =
        [&](const AnimationNode* node, const glm::mat4& parent)
        {
            const std::string& nodeName = node->name;

            glm::mat4 local = node->transformation;

            auto it = anim.animationData.m_BoneInfoMap.find(nodeName);
            if (it != anim.animationData.m_BoneInfoMap.end())
            {
                int idx = it->second.id;
                if (idx < NUM_BONES && localPose[idx].valid)
                    local = ComposeTRS(localPose[idx]);
            }

            glm::mat4 global = parent * local;

            if (it != anim.animationData.m_BoneInfoMap.end())
            {
                int idx = it->second.id;
                if (idx < NUM_BONES)
                {
                    glm::mat4 offset = it->second.offset;
                    m_FinalBoneMatrices->at(idx) = global * offset;
                }
            }

            for (int i = 0; i < node->childrenCount; ++i)
                dfs(&node->children[i], global);
        };

    auto root = anim.GetRootNode();
    dfs(&root, glm::mat4(1.0f));
}
