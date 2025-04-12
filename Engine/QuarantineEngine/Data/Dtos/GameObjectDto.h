#pragma once

#ifndef GAME_OBJECT_DTO_H
#define GAME_OBJECT_DTO_H

#include <string>
#include <glm/glm.hpp>
#include <MeshImportedType.h>

#pragma pack(1)
struct GameObjectDto
{
    std::string Id;
    std::string Name;
    glm::mat4 WorldTransform;
    int MeshImportedType;
    int MeshPrimitiveType;
    std::string MeshPath;

    GameObjectDto()
        : Id{},
        Name{},
        WorldTransform(glm::mat4(1.0f)),
        MeshImportedType(NONE_GEO),
        MeshPrimitiveType(-1),
        MeshPath{}
    {
    }

    GameObjectDto(std::string id, std::string name, glm::mat4 worldTransform, int meshImportedType, int meshPrimitiveType, std::string meshPath)
    {
        this->Id = id;
        this->Name = name;
        this->WorldTransform = worldTransform;
        this->MeshImportedType = meshImportedType;
        this->MeshPrimitiveType = meshPrimitiveType;
        this->MeshPath = meshPath;
    }
};
#pragma pack()

#endif
