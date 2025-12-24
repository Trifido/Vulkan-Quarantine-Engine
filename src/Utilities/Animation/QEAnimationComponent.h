#pragma once

#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H
#include "QEGameComponent.h"
#include <Animation/Animation.h>
#include <Animator.h>
#include <QEAnimationStateData.h>

class QEAnimationComponent : public QEGameComponent
{
    REFLECTABLE_COMPONENT(QEAnimationComponent)
public:
    std::shared_ptr<Animator> animator;

private:
    REFLECT_PROPERTY(std::string, _entryStateId)
    REFLECT_PROPERTY(AnimationState, currentState)
    REFLECT_PROPERTY(std::vector<AnimationState>, States)
    std::unordered_map<std::string, AnimationState> _states;
    REFLECT_PROPERTY(std::vector<QETransition>, Transitions)
    std::unordered_map<std::string, std::vector<QETransition>> _transitionsFrom;

    std::unordered_map<std::string, std::shared_ptr<Animation>> _animations;
    size_t numAnimations = 0;
    std::unordered_map<std::string, QEParam> _params;

    std::unordered_set<std::string> _triggersUsedThisFrame;

private:
    QEParam& ensureParam_(const std::string& name, QEParamType desired);
    void ClearAllTriggers();
    void ChangeState(const std::string& toId);
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

    void AddAnimationState(AnimationState state, bool isEntryState = false);
    void AddTransition(const QETransition& t);
    void SetBool(const std::string& name, bool v);
    void SetInt(const std::string& name, int v);
    void SetFloat(const std::string& name, float v);
    void SetTrigger(const std::string& name, bool value = true);

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

#endif
