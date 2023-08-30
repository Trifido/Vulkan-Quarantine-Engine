#pragma once

#ifndef GAMECOMPONENT_H
#define GAMECOMPONENT_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Numbered.h>

class GameComponent : Numbered
{
public:
    GameComponent() {}
};

#endif
