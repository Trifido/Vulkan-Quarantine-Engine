#pragma once
#include "imgui.h"
#include <glm/glm.hpp>
class SceneWindow
{
private:
    glm::vec2 mSize;
public:
    void renderScene();
};

