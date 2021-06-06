#pragma once

#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>
#include "GameComponent.h"
#include "Texture.h"
#include "CommandPoolModule.h"
#include "BufferManageModule.h"

class Material : public GameComponent
{
private:
    CommandPoolModule* commandPoolModule;
    BufferManageModule* bufferManagerModule;
public:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 emission;

    std::unique_ptr<Texture> albedo;

public:
    Material();
    void addAlbedo(std::string path);
    Texture getAlbedo() { return *albedo; }

    void cleanup();
};

#endif // !MATERIAL_H



