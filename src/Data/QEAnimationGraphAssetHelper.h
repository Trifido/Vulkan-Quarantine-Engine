#pragma once

#ifndef QE_ANIMATION_GRAPH_ASSET_HELPER_H
#define QE_ANIMATION_GRAPH_ASSET_HELPER_H

#include <filesystem>

class QEAnimationComponent;
namespace YAML { class Node; }

namespace QEAnimationGraphAssetHelper
{
    YAML::Node SerializeAnimationComponentReference(const QEAnimationComponent& component);

    bool LoadAnimationComponentFromReference(
        QEAnimationComponent& component,
        const YAML::Node& componentNode);
}

#endif // !QE_ANIMATION_GRAPH_ASSET_HELPER_H
