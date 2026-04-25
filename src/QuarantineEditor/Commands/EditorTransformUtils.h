#pragma once

#include <QETransform.h>
#include "TransformCommand.h"

namespace EditorTransformUtils
{
    inline TransformState CaptureState(const std::shared_ptr<QETransform>& transform)
    {
        TransformState state{};

        if (!transform)
            return state;

        state.Position = transform->localPosition;
        state.Rotation = transform->localRotation;
        state.Scale = transform->localScale;

        return state;
    }
}
