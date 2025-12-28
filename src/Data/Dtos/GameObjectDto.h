#pragma once

#ifndef GAME_OBJECT_DTO_H
#define GAME_OBJECT_DTO_H

#include <string>
#include <glm/glm.hpp>

#pragma pack(1)
struct GameObjectDto
{
    std::string Id;
    std::string Name;
    glm::mat4 WorldTransform;
    std::string MeshPath;
    std::string BindMaterialName;

    GameObjectDto()
        : Id{},
        Name{},
        WorldTransform(glm::mat4(1.0f)),
        MeshPath{},
        BindMaterialName{}
    {
    }

    GameObjectDto(std::string id, std::string name, glm::mat4 worldTransform, std::string meshPath, std::string materialName)
    {
        this->Id = id;
        this->Name = name;
        this->WorldTransform = worldTransform;
        this->MeshPath = meshPath;
        this->BindMaterialName = materialName;
    }
};
#pragma pack()

#endif
