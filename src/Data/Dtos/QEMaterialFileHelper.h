#pragma once

#ifndef QE_MATERIAL_FILE_HELPER_H
#define QE_MATERIAL_FILE_HELPER_H

#include <filesystem>
#include <string>
#include "MaterialDto.h"

class QEMaterialFileHelper
{
public:
    static bool WriteMaterialFile(const std::filesystem::path& filePath, const MaterialDto& dto);

    static MaterialDto BuildDefaultPrimitiveMaterialDto(const std::filesystem::path& projectRoot);
    static MaterialDto BuildDefaultParticlesMaterialDto(const std::filesystem::path& projectRoot);
    static MaterialDto BuildEditorDebugAABBMaterialDto(const std::filesystem::path& projectRoot);
    static MaterialDto BuildEditorDebugColliderMaterialDto(const std::filesystem::path& projectRoot);
    static MaterialDto BuildEditorGridMaterialDto(const std::filesystem::path& projectRoot);

private:
    static MaterialDto BuildBaseMaterialDto(
        const std::string& materialName,
        const std::string& shaderName,
        const std::filesystem::path& projectRoot);

    static std::string ToProjectRelativePath(
        const std::filesystem::path& absolutePath,
        const std::filesystem::path& projectRoot);
};

#endif
