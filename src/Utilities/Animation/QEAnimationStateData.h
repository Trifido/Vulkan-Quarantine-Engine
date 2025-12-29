#pragma once

#ifndef ANIMATION_STATE_DATA_H
#define ANIMATION_STATE_DATA_H

#include <string>
#include <vector>
#include <iostream>

enum class QEParamType { Bool, Int, Float, Trigger };

struct QEParam
{
    QEParamType type;
    union { bool b; int i; float f; } value{};
    bool trigger = false;
};

enum class QEOp { Equal, NotEqual, Greater, Less, GreaterEqual, LessEqual };

struct QECondition
{
    std::string param;
    QEOp op;
    float value = 0.0f; // int/float; bool 0/1
};

struct QETransition
{
    std::string fromState;
    std::string toState;
    std::vector<QECondition> conditions;

    int   priority = 0;           // greater = first
    bool  hasExitTime = false;
    float exitTimeNormalized = 1.0f; // [0..1], p.ej 0.8
    float blendDuration = 0.2f;
};

struct AnimationState
{
    std::string Id;
    bool Loop = true;
    std::string AnimationClip; // name in _animations
};

inline void LogTypeChange(const std::string& name, const char* fromT, const char* toT)
{
    std::cerr << "[Animator] Param '" << name << "' cambiado de " << fromT << " a " << toT << "\n";
}

#endif // !ANIMATION_STATE_DATA_H
