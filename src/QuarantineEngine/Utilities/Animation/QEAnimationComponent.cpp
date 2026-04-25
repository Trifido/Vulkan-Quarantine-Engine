#include "QEAnimationComponent.h"

#include <QEGameObject.h>
#include <Timer.h>
#include <Logging/QELogMacros.h>

QEAnimationComponent::QEAnimationComponent()
{
    this->animator = std::make_shared<Animator>();
    this->AutoStart = false;
}

void QEAnimationComponent::RebuildStateMachineCaches()
{
    _states.clear();
    for (const auto& state : States)
    {
        _states[state.Id] = state;
    }

    _transitionsFrom.clear();
    for (const auto& transition : Transitions)
    {
        _transitionsFrom[transition.fromState].push_back(transition);
    }

    for (auto& [_, transitions] : _transitionsFrom)
    {
        std::sort(transitions.begin(), transitions.end(), [](const auto& a, const auto& b) { return a.priority > b.priority; });
    }
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

std::vector<std::string> QEAnimationComponent::GetAnimationClipNames() const
{
    std::vector<std::string> names;
    names.reserve(_animations.size());

    for (const auto& [name, _] : _animations)
    {
        names.push_back(name);
    }

    std::sort(names.begin(), names.end());
    return names;
}

void QEAnimationComponent::AddAnimationState(AnimationState state, bool isEntryState)
{
    if (this->_states.find(state.Id) == this->_states.end())
    {
        this->_states[state.Id] = state;
        States.push_back(state);

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
    Transitions.push_back(t);
}

const std::vector<AnimationState>& QEAnimationComponent::GetAnimationStates() const
{
    return States;
}

const std::vector<QETransition>& QEAnimationComponent::GetAnimationTransitions() const
{
    return Transitions;
}

const std::vector<std::string>& QEAnimationComponent::GetBoolParameterNames() const
{
    return BoolParameters;
}

const std::vector<std::string>& QEAnimationComponent::GetIntParameterNames() const
{
    return IntParameters;
}

const std::vector<std::string>& QEAnimationComponent::GetFloatParameterNames() const
{
    return FloatParameters;
}

const std::vector<std::string>& QEAnimationComponent::GetTriggerParameterNames() const
{
    return TriggerParameters;
}

bool QEAnimationComponent::TryGetDeclaredParameterType(const std::string& name, QEParamType& outType) const
{
    if (std::find(BoolParameters.begin(), BoolParameters.end(), name) != BoolParameters.end())
    {
        outType = QEParamType::Bool;
        return true;
    }

    if (std::find(IntParameters.begin(), IntParameters.end(), name) != IntParameters.end())
    {
        outType = QEParamType::Int;
        return true;
    }

    if (std::find(FloatParameters.begin(), FloatParameters.end(), name) != FloatParameters.end())
    {
        outType = QEParamType::Float;
        return true;
    }

    if (std::find(TriggerParameters.begin(), TriggerParameters.end(), name) != TriggerParameters.end())
    {
        outType = QEParamType::Trigger;
        return true;
    }

    return false;
}

void QEAnimationComponent::SetStateMachineData(
    const std::vector<AnimationState>& states,
    const std::vector<QETransition>& transitions,
    const std::string& entryStateId)
{
    States = states;
    Transitions = transitions;

    RebuildStateMachineCaches();

    currentState = AnimationState{};

    if (!entryStateId.empty())
    {
        auto entryIt = _states.find(entryStateId);
        if (entryIt != _states.end())
        {
            currentState = entryIt->second;
            return;
        }
    }

    if (!States.empty())
    {
        currentState = States.front();
    }

    _activeTransitionFromState.clear();
    _activeTransitionToState.clear();
    _hasStartedPlayback = false;
}

void QEAnimationComponent::SetBoolParameterNames(const std::vector<std::string>& boolNames)
{
    BoolParameters = boolNames;

    for (const auto& paramName : BoolParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Bool);
        }
    }
}

void QEAnimationComponent::SetIntParameterNames(const std::vector<std::string>& intNames)
{
    IntParameters = intNames;

    for (const auto& paramName : IntParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Int);
        }
    }
}

void QEAnimationComponent::SetFloatParameterNames(const std::vector<std::string>& floatNames)
{
    FloatParameters = floatNames;

    for (const auto& paramName : FloatParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Float);
        }
    }
}

void QEAnimationComponent::SetTriggerParameterNames(const std::vector<std::string>& triggerNames)
{
    TriggerParameters = triggerNames;

    for (const auto& triggerName : TriggerParameters)
    {
        if (!triggerName.empty())
        {
            ensureParam_(triggerName, QEParamType::Trigger);
        }
    }
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
            "Trigger";
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
    if (it == _states.end())
    {
        QE_LOG_ERROR_CAT_F("QEAnimationComponent", "State not found: {}", toId);
        return;
    }

    currentState = it->second;
    _activeTransitionFromState.clear();
    _activeTransitionToState.clear();

    ClearAllTriggers();

    auto clip = GetAnimation(currentState.AnimationClip);
    if (clip)
    {
        animator->PlayAnimation(clip, it->second.Loop);
    }
}

void QEAnimationComponent::ChangeState(const std::string& toId, const QETransition& tr)
{
    auto it = _states.find(toId);
    if (it == _states.end())
    {
        QE_LOG_ERROR_CAT_F("QEAnimationComponent", "State not found: {}", toId);
        return;
    }

    const bool prevLoop = currentState.Loop;
    const std::string previousStateId = currentState.Id;
    AnimationState nextState = it->second;

    auto nextClip = GetAnimation(nextState.AnimationClip);
    if (!nextClip)
    {
        QE_LOG_ERROR_CAT_F("QEAnimationComponent", "Clip not found: {}", nextState.AnimationClip);
        return;
    }
    _activeTransitionFromState = previousStateId;
    _activeTransitionToState = nextState.Id;

    animator->CrossFadeTo(nextClip, tr.blendDuration, nextState.Loop, prevLoop);

    if (!animator->IsInTransition())
    {
        currentState = nextState;
        _activeTransitionFromState.clear();
        _activeTransitionToState.clear();
    }

    ClearAllTriggers();
}

bool QEAnimationComponent::CheckCondition(const QECondition& c)
{
    auto it = _params.find(c.param);
    if (it == _params.end()) return false;

    const QEParam& p = it->second;
    auto cmp = [&](float a, float b)
        {
            switch (c.op)
            {
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
    for (auto& c : t.conditions)
    {
        if (!CheckCondition(c))
            return false;
    }
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

        QE_LOG_INFO_CAT_F("QEAnimationComponent", "ToState: {}", t.toState);

        return &t;
    }
    return nullptr;
}

void QEAnimationComponent::ConsumeTriggersUsed(const QETransition& t)
{
    for (const auto& c : t.conditions)
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
}

void QEAnimationComponent::SetGraphAssetPath(const std::string& path)
{
    GraphAssetPath = path;
}

const std::string& QEAnimationComponent::GetGraphAssetPath() const
{
    return GraphAssetPath;
}

void QEAnimationComponent::SetAutoStart(bool autoStart)
{
    AutoStart = autoStart;
}

bool QEAnimationComponent::GetAutoStart() const
{
    return AutoStart;
}

void QEAnimationComponent::StartEntryPlayback()
{
    auto entryState = _states.find(currentState.Id);
    if (entryState != _states.end())
    {
        auto entryAnimation = _animations.find(entryState->second.AnimationClip);
        if (entryAnimation != _animations.end())
        {
            animator->PlayAnimation(entryAnimation->second, entryState->second.Loop);
            _hasStartedPlayback = true;
            _activeTransitionFromState.clear();
            _activeTransitionToState.clear();
            return;
        }
    }

    auto anim = _animations.begin();
    if (anim != _animations.end())
    {
        this->animator->PlayAnimation(anim->second, true);
        _hasStartedPlayback = true;
    }
}

bool QEAnimationComponent::HasStartedPlayback() const
{
    return _hasStartedPlayback;
}

AnimationState QEAnimationComponent::GetCurrentState() const
{
    return currentState;
}

bool QEAnimationComponent::IsInStateTransition() const
{
    return animator && animator->IsInTransition() &&
        !_activeTransitionFromState.empty() &&
        !_activeTransitionToState.empty();
}

std::string QEAnimationComponent::GetActiveTransitionFromState() const
{
    return _activeTransitionFromState;
}

std::string QEAnimationComponent::GetActiveTransitionToState() const
{
    return _activeTransitionToState;
}

bool QEAnimationComponent::GetBool(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return false;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Bool)
    {
        QE_LOG_WARN_CAT_F("QEAnimationComponent", "GetBool({}) called on a non-bool parameter.", name);
        return false;
    }
    return p.value.b;
}

int QEAnimationComponent::GetInt(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return 0;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Int)
    {
        QE_LOG_INFO_CAT_F("QEAnimationComponent", "GetInt({}) called on a non-int parameter.", name);
        return 0;
    }
    return p.value.i;
}

float QEAnimationComponent::GetFloat(const std::string& name) const
{
    auto it = _params.find(name);
    if (it == _params.end()) return 0.0f;

    const QEParam& p = it->second;
    if (p.type != QEParamType::Float)
    {
        QE_LOG_WARN_CAT_F("QEAnimationComponent", "GetFloat({}) called on a non-float parameter.", name);
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
        QE_LOG_WARN_CAT_F("QEAnimationComponent", "IsTriggerSet({}) called on a non-trigger parameter.", name);
        return false;
    }
    return p.trigger;
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
    if (!_hasStartedPlayback)
    {
        return;
    }

    animator->UpdateAnimation(Timer::DeltaTime, currentState.Loop);

    if (!animator->IsInTransition())
    {
        if (!_activeTransitionToState.empty())
        {
            auto transitionedState = _states.find(_activeTransitionToState);
            if (transitionedState != _states.end())
            {
                currentState = transitionedState->second;
            }

            _activeTransitionFromState.clear();
            _activeTransitionToState.clear();
            _triggersUsedThisFrame.clear();
            return;
        }

        if (auto* tr = FindValidTransition())
        {
            ConsumeTriggersUsed(*tr);
            ChangeState(tr->toState, *tr);
        }
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
    if (_states.empty() || _transitionsFrom.empty())
    {
        RebuildStateMachineCaches();
    }

    for (const auto& paramName : BoolParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Bool);
        }
    }

    for (const auto& paramName : IntParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Int);
        }
    }

    for (const auto& paramName : FloatParameters)
    {
        if (!paramName.empty())
        {
            ensureParam_(paramName, QEParamType::Float);
        }
    }

    for (const auto& triggerName : TriggerParameters)
    {
        if (!triggerName.empty())
        {
            ensureParam_(triggerName, QEParamType::Trigger);
        }
    }

    _hasStartedPlayback = false;
    if (AutoStart)
    {
        StartEntryPlayback();
    }
    else
    {
        // In editor we still want the mesh posed, even if the state machine
        // should wait for an explicit start.
        StartEntryPlayback();
        _hasStartedPlayback = false;
    }

    QEGameComponent::QEInit();
}
