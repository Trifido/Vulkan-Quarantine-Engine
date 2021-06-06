#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "Texture.h"

class Material : public GameComponent
{
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emission;

    std::unique_ptr<Texture> albedo;

public:
    Material();
    void addAlbedo(std::string path, VkCommandPool& commandPool);
    Texture getAlbedo() { return *albedo; }

    void cleanup();
};

#endif // !MATERIAL_H



