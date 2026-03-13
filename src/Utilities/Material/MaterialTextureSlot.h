#pragma once
#ifndef MATERIAL_TEXTURE_SLOT_H
#define MATERIAL_TEXTURE_SLOT_H

enum class MAT_TEX_SLOT : int
{
    BaseColor = 0,
    Normal = 1,
    Metallic = 2,
    Roughness = 3,
    AO = 4,
    Emissive = 5,
    Height = 6,
    Reserved7 = 7,
    Count = 8
};

static int SlotFromType(TEXTURE_TYPE t)
{
    switch (t)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:   return (int)MAT_TEX_SLOT::BaseColor;
    case TEXTURE_TYPE::NORMAL_TYPE:    return (int)MAT_TEX_SLOT::Normal;
    case TEXTURE_TYPE::METALNESS_TYPE: return (int)MAT_TEX_SLOT::Metallic;
    case TEXTURE_TYPE::ROUGHNESS_TYPE: return (int)MAT_TEX_SLOT::Roughness;
    case TEXTURE_TYPE::AO_TYPE:        return (int)MAT_TEX_SLOT::AO;
    case TEXTURE_TYPE::EMISSIVE_TYPE:  return (int)MAT_TEX_SLOT::Emissive;
    case TEXTURE_TYPE::HEIGHT_TYPE:    return (int)MAT_TEX_SLOT::Height;
    case TEXTURE_TYPE::SPECULAR_TYPE:  return (int)MAT_TEX_SLOT::Reserved7;
    default:                            return -1;
    }
}

#endif
