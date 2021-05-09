#pragma once
#include "../../external/imgui/imgui.h"
#include <glm/glm.hpp>
class SceneWindow
{
private:
    glm::vec2 mSize;
public:
    void renderScene();
};

