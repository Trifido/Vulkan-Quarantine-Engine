#include "QEAnimationComponent.h"
#include <QEGameObject.h>
#include <Timer.h>

QEAnimationComponent::QEAnimationComponent()
{
    this->animator = std::make_shared<Animator>();
}

void QEAnimationComponent::AddAnimation(std::shared_ptr<Animation> animation_ptr)
{
    if (this->_animations.find(animation_ptr->name) == _animations.end())
    {
        this->_animations[animation_ptr->name] = animation_ptr;
    }
    else
    {
        this->_animations[animation_ptr->name + "_1"] = animation_ptr;
    }

    this->numAnimations++;
}

void QEAnimationComponent::AddAnimation(Animation animation)
{
    if (this->_animations.find(animation.name) == _animations.end())
    {
        this->_animations[animation.name] = std::make_shared<Animation>(animation);
    }
    else
    {
        this->_animations[animation.name + "_1"] = std::make_shared<Animation>(animation);
    }

    this->numAnimations++;
}

std::shared_ptr<Animation> QEAnimationComponent::GetAnimation(std::string name)
{
    auto it = _animations.find(name);

    if (it != _animations.end())
        return it->second;

    return nullptr;
}

void QEAnimationComponent::AddAnimationState(AnimationState state, bool isEntryState)
{
    if (this->_states.find(state.Id) == this->_states.end())
    {
        this->_states[state.Id] = state;

        if (isEntryState)
        {
            this->currentState = state;
        }
    }
}

void QEAnimationComponent::AddTransition(const QETransition& t)
{
    _transitionsFrom[t.fromState].push_back(t);
    auto& v = _transitionsFrom[t.fromState];
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.priority > b.priority; });
}

QEParam& QEAnimationComponent::ensureParam_(const std::string& name, QEParamType desired)
{
    auto it = _params.find(name);
    if (it == _params.end())
    {
        QEParam p{};
        p.type = desired;
        p.value.i = 0;
        p.trigger = false;
        auto [it2, _] = _params.emplace(name, p);
        return it2->second;
    }

    QEParam& p = it->second;
    if (p.type != desired)
    {
        const char* fromT =
            p.type == QEParamType::Bool ? "Bool" :
            p.type == QEParamType::Int ? "Int" :
            p.type == QEParamType::Float ? "Float" :
            /*Trigger*/                      "Trigger";
        const char* toT =
            desired == QEParamType::Bool ? "Bool" :
            desired == QEParamType::Int ? "Int" :
            desired == QEParamType::Float ? "Float" :
            "Trigger";

        LogTypeChange(name, fromT, toT);
        p.type = desired;
        p.value.i = 0;
        p.trigger = false;
    }
    return p;
}

void QEAnimationComponent::ClearAllTriggers()
{
    for (auto& kv : _params)
    {
        QEParam& p = kv.second;
        if (p.type == QEParamType::Trigger)
            p.trigger = false;
    }
}

void QEAnimationComponent::ChangeState(const std::string& toId)
{
    auto it = _states.find(toId);
    if (it == _states.end()) { std::cerr << "State not found: " << toId << "\n"; return; }

    currentState = it->second;

    ClearAllTriggers();

    auto clip = GetAnimation(currentState.AnimationClip);
    if (clip)
    {
        animator->PlayAnimation(clip);
    }
}

bool QEAnimationComponent::CheckCondition(const QECondition& c)
{
    auto it = _params.find(c.param);
    if (it == _params.end()) return false;

    const QEParam& p = it->second;
    auto cmp = [&](float a, float b)
        {
        switch (c.op) {
        case QEOp::Equal:         return a == b;
        case QEOp::NotEqual:      return a != b;
        case QEOp::Greater:       return a > b;
        case QEOp::Less:          return a < b;
        case QEOp::GreaterEqual:  return a >= b;
        case QEOp::LessEqual:     return a <= b;
        }
        return false;
        };

    switch (p.type)
    {
        case QEParamType::Bool:    return cmp(p.value.b ? 1.f : 0.f, c.value);
        case QEParamType::Int:     return cmp((float)p.value.i, c.value);
        case QEParamType::Float:   return cmp(p.value.f, c.value);
        case QEParamType::Trigger:
        {
            if (p.trigger)
                _triggersUsedThisFrame.insert(c.param);

            return p.trigger;
        }
    }

    return false;
}

bool QEAnimationComponent::AreAllConditionsTrue(const QETransition& t)
{
    for (auto& c : t.conditions) if (!CheckCondition(c)) return false;
    return true;
}

bool QEAnimationComponent::ExitTimeOk(const QETransition& t, const AnimationState& st)
{
    if (!t.hasExitTime) return true;
    float dur = this->animator->GetDurationTicks();
    if (dur <= 0.f) return true;

    if (!st.Loop)
    {
        const float eps = std::max(1e-4f, dur * 1e-5f);
        return this->animator->GetTimeTicks() >= (dur - eps);
    }
    else
    {
        return this->animator->GetNormalizedTime() >= t.exitTimeNormalized;
    }
}

const QETransition* QEAnimationComponent::FindValidTransition()
{
    auto it = _transitionsFrom.find(currentState.Id);
    if (it == _transitionsFrom.end()) return nullptr;

    for (const auto& t : it->second)
    {
        if (!ExitTimeOk(t, currentState)) continue;
        if (!AreAllConditionsTrue(t)) continue;
        std::cout << "ToState: " + t.toState << std::endl;
        return &t;
    }
    return nullptr;
}

void QEAnimationComponent::ConsumeTriggersUsed(const QETransition& t)
{
    for(const auto& c : t.conditions)
    {
        auto it = _params.find(c.param);
        if (it != _params.end() && it->second.type == QEParamType::Trigger)
        {
            it->second.trigger = false;
        }
    }
}

void QEAnimationComponent::SetBool(const std::string& name, bool v)
{
    QEParam& p = ensureParam_(name, QEParamType::Bool);
    p.value.b = v;
}

void QEAnimationComponent::SetInt(const std::string& name, int v)
{
    QEParam& p = ensureParam_(name, QEParamType::Int);
    p.value.i = v;
}

void QEAnimationComponent::SetFloat(const std::string& name, float v)
{
    QEParam& p = ensureParam_(name, QEParamType::Float);
    p.value.f = v;
}

void QEAnimationComponent::SetTrigger(const std::string& name, bool value)
{
    QEParam& p = ensureParam_(name, QEParamType::Trigger);
    p.trigger = value;
    std::cout << "Set Trigger\n";
}

bool QEAnimationComponent::GetBool(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return false;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Bool) {
        std::cerr << "[Animator] Warning: GetBool('" << name
            << "') llamado sobre parámetro no-bool.\n";
        return false;
    }
    return p.value.b;
}

int QEAnimationComponent::GetInt(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return 0;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Int) {
        std::cerr << "[Animator] Warning: GetInt('" << name
            << "') llamado sobre parámetro no-int.\n";
        return 0;
    }
    return p.value.i;
}

float QEAnimationComponent::GetFloat(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return 0.0f;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Float) {
        std::cerr << "[Animator] Warning: GetFloat('" << name
            << "') llamado sobre parámetro no-float.\n";
        return 0.0f;
    }
    return p.value.f;
}

bool QEAnimationComponent::IsTriggerSet(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return false;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Trigger)
    {
        std::cerr << "[Animator] Warning: IsTriggerSet('" << name
            << "') llamado sobre parámetro no-trigger.\n";
        return false;
    }
    return p.trigger; // true si el trigger está activo
}

void QEAnimationComponent::CleanLastResources()
{
    if (this->animator != nullptr)
    {
        this->animator->CleanAnimatorUBO();
        this->animator.reset();
        this->animator = nullptr;

        this->_animations.clear();
        this->numAnimations = 0;
    }
}

void QEAnimationComponent::QEStart()
{
    auto animationComponent = this->Owner->GetComponent<QEAnimationComponent>();
    if (animationComponent != nullptr)
    {
        auto meshComponent = this->Owner->GetComponent<QEGeometryComponent>();
        auto mesh = meshComponent->GetMesh();

        if (mesh->MeshData.empty())
        {
            return;
        }

        std::vector<std::string> idMeshes;
        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            idMeshes.push_back(std::to_string(i));
        }

        animationComponent->animator->InitializeComputeNodes(idMeshes);

        for (unsigned int i = 0; i < mesh->MeshData.size(); i++)
        {
            animationComponent->animator->SetVertexBufferInComputeNode(idMeshes[i], meshComponent->vertexBuffer[i], meshComponent->animationBuffer[i], mesh->MeshData[i].NumVertices);
        }

        animationComponent->animator->InitializeDescriptorsComputeNodes();
    }

    QEGameComponent::QEStart();
}

void QEAnimationComponent::QEUpdate()
{
    this->animator->UpdateAnimation(Timer::DeltaTime, currentState.Loop);

    if (auto* tr = FindValidTransition())
    {
        ConsumeTriggersUsed(*tr);
        ChangeState(tr->toState);
    }

    for (const std::string& tname : _triggersUsedThisFrame)
    {
        auto it = _params.find(tname);
        if (it != _params.end() && it->second.type == QEParamType::Trigger)
            it->second.trigger = false;
    }
    _triggersUsedThisFrame.clear();
}

void QEAnimationComponent::QEDestroy()
{
    this->CleanLastResources();
    QEGameComponent::QEDestroy();
}

void QEAnimationComponent::QEInit()
{
    auto entryState = this->_states.find(this->currentState.Id);
    if (entryState != this->_states.end())
    {
        string animationClip = entryState->second.AnimationClip;
        auto entryAnimation  = this->_animations.find(animationClip);

        if (entryAnimation != this->_animations.end())
        {
            this->animator->PlayAnimation(entryAnimation->second);
        }
    }

    QEGameComponent::QEInit();
}
