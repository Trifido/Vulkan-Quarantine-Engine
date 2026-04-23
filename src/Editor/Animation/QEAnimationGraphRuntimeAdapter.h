#pragma once

#ifndef QE_ANIMATION_GRAPH_RUNTIME_ADAPTER_H
#define QE_ANIMATION_GRAPH_RUNTIME_ADAPTER_H

#include <Editor/Animation/QEAnimationGraphEditorData.h>

class QEAnimationComponent;

namespace QEAnimationGraphRuntimeAdapter
{
    QEAnimationGraphEditorData BuildEditorData(
        const QEAnimationComponent& component,
        const QEAnimationGraphEditorData* previousEditorData = nullptr);

    void ApplyEditorData(
        QEAnimationComponent& component,
        const QEAnimationGraphEditorData& editorData);
}

#endif // !QE_ANIMATION_GRAPH_RUNTIME_ADAPTER_H
