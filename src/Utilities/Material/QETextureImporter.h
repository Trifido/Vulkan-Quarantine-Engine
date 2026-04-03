#pragma once

#ifndef QE_TEXTURE_IMPORTER_H
#define QE_TEXTURE_IMPORTER_H

#include <string>
#include <filesystem>
#include <TextureTypes.h>
#include <CustomTexture.h>

enum class QETextureCompressionPreset
{
    UASTC,
    ETC1S
};

struct QETextureImportSettings
{
    TEXTURE_TYPE semantic = TEXTURE_TYPE::NULL_TYPE;
    QEColorSpace colorSpace = QEColorSpace::SRGB;
    QETextureCompressionPreset compressionPreset = QETextureCompressionPreset::UASTC;
    bool generateMipmaps = true;
    bool overwrite = false;
};

struct QETextureImportResult
{
    bool success = false;
    std::string sourcePath;
    std::string importedPath;
    std::string error;
};

class QETextureImporter
{
public:
    static std::string BuildImportedPath(const std::string& sourcePath);

    static QETextureImportResult ImportToKtx2(
        const std::string& sourcePath,
        const std::string& outputPath,
        const QETextureImportSettings& settings);

private:
    static std::string Quote(const std::string& s);
};

#endif
