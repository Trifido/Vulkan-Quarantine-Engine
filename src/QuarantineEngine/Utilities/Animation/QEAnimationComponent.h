#pragma once

#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H
#include "QEGameComponent.h"
#include <Animation/Animation.h>
#include <Animator.h>
#include <QEAnimationStateData.h>

class QEAnimationComponent : public QEGameComponent
{
public:
    REFLECTABLE_COMPONENT(QEAnimationComponent)
    std::shared_ptr<Animator> animator;

private:
    REFLECT_PROPERTY(std::string, GraphAssetPath)
    REFLECT_PROPERTY(bool, AutoStart)
    REFLECT_PROPERTY(AnimationState, currentState)
    REFLECT_PROPERTY(std::vector<AnimationState>, States)
    std::unordered_map<std::string, AnimationState> _states;
    REFLECT_PROPERTY(std::vector<QETransition>, Transitions)
    REFLECT_PROPERTY(std::vector<std::string>, BoolParameters)
    REFLECT_PROPERTY(std::vector<std::string>, IntParameters)
    REFLECT_PROPERTY(std::vector<std::string>, FloatParameters)
    REFLECT_PROPERTY(std::vector<std::string>, TriggerParameters)
    std::unordered_map<std::string, std::vector<QETransition>> _transitionsFrom;

    std::unordered_map<std::string, std::shared_ptr<Animation>> _animations;
    size_t numAnimations = 0;
    std::unordered_map<std::string, QEParam> _params;

    std::unordered_set<std::string> _triggersUsedThisFrame;
    std::string _activeTransitionFromState;
    std::string _activeTransitionToState;
    bool _hasStartedPlayback = false;

private:
    QEParam& ensureParam_(const std::string& name, QEParamType desired);
    void RebuildStateMachineCaches();
    void ClearAllTriggers();
    void ChangeState(const std::string& toId);
    void ChangeState(const std::string& toId, const QETransition& tr);
    bool CheckCondition(const QECondition& c);
    bool AreAllConditionsTrue(const QETransition& t);
    const QETransition* FindValidTransition();
    bool ExitTimeOk(const QETransition& t, const AnimationState& st);
    void ConsumeTriggersUsed(const QETransition& t);

public:
    QEAnimationComponent();
    void AddAnimation(std::shared_ptr<Animation> animation_ptr);
    void AddAnimation(Animation animation);
    std::shared_ptr<Animation> GetAnimation(std::string name);
    std::vector<std::string> GetAnimationClipNames() const;

    void AddAnimationState(AnimationState state, bool isEntryState = false);
    void AddTransition(const QETransition& t);
    const std::vector<AnimationState>& GetAnimationStates() const;
    const std::vector<QETransition>& GetAnimationTransitions() const;
    const std::vector<std::string>& GetBoolParameterNames() const;
    const std::vector<std::string>& GetIntParameterNames() const;
    const std::vector<std::string>& GetFloatParameterNames() const;
    const std::vector<std::string>& GetTriggerParameterNames() const;
    bool TryGetDeclaredParameterType(const std::string& name, QEParamType& outType) const;
    void SetStateMachineData(
        const std::vector<AnimationState>& states,
        const std::vector<QETransition>& transitions,
        const std::string& entryStateId);
    void SetBoolParameterNames(const std::vector<std::string>& boolNames);
    void SetIntParameterNames(const std::vector<std::string>& intNames);
    void SetFloatParameterNames(const std::vector<std::string>& floatNames);
    void SetTriggerParameterNames(const std::vector<std::string>& triggerNames);
    void SetBool(const std::string& name, bool v);
    void SetInt(const std::string& name, int v);
    void SetFloat(const std::string& name, float v);
    void SetTrigger(const std::string& name, bool value = true);
    void SetGraphAssetPath(const std::string& path);
    const std::string& GetGraphAssetPath() const;
    void SetAutoStart(bool autoStart);
    bool GetAutoStart() const;
    void StartEntryPlayback();
    bool HasStartedPlayback() const;

    AnimationState GetCurrentState() const;
    bool IsInStateTransition() const;
    std::string GetActiveTransitionFromState() const;
    std::string GetActiveTransitionToState() const;
    bool GetBool(const std::string& name) const;
    int  GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;
    bool IsTriggerSet(const std::string& name) const;

    void CleanLastResources();

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};



namespace QE
{
    using ::QEAnimationComponent;
} // namespace QE
// QE namespace aliases
#endif
