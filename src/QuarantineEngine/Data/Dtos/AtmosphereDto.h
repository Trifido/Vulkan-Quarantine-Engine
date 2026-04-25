#pragma once
#ifndef ATMOSPHERE_DTO_H
#define ATMOSPHERE_DTO_H

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm_yaml_conversions.h>

#pragma pack(push, 1)
struct AtmosphereDto
{
    // --- CORE ---
    bool hasAtmosphere;
    int environmentType;

    glm::vec3 sunEulerDegrees;
    float sunBaseIntensity;

    // --- SCATTERING ---
    glm::vec3 rayleighScattering;
    float rayleighScaleHeight;

    glm::vec3 mieScattering;
    float mieScaleHeight;

    float mieAnisotropy;

    // --- ABSORPTION ---
    glm::vec3 ozoneAbsorption;
    float ozoneDensity;

    // --- PLANET ---
    float planetRadius;
    float atmosphereRadius;

    // --- LIGHT ---
    glm::vec3 sunColor;
    float sunIntensityMultiplier;

    // --- ARTISTIC ---
    float exposure;
    float skyTint;
    float horizonSoftness;
    float multiScatteringFactor;

    // --- SUN DISC / BLOOM ---
    float sunDiskSize;
    float sunDiskIntensity;
    float sunGlow;

    // --- DEFAULT CONSTRUCTOR ---
    AtmosphereDto()
        : hasAtmosphere(true),
        environmentType(2),

        sunEulerDegrees{ -45.0f, 45.0f, 0.0f },
        sunBaseIntensity(100.0f),

        rayleighScattering{ 5.802f, 13.558f, 33.100f },
        rayleighScaleHeight(8000.0f),

        mieScattering{ 3.996f, 3.996f, 3.996f },
        mieScaleHeight(1200.0f),
        mieAnisotropy(0.8f),

        ozoneAbsorption{ 0.650f, 1.881f, 0.085f },
        ozoneDensity(1.0f),

        planetRadius(6360000.0f),
        atmosphereRadius(6460000.0f),

        sunColor{ 1.0f, 0.98f, 0.92f },
        sunIntensityMultiplier(1.0f),

        exposure(10.0f),
        skyTint(1.0f),
        horizonSoftness(1.0f),
        multiScatteringFactor(1.0f),

        sunDiskSize(1.0f),
        sunDiskIntensity(1.0f),
        sunGlow(1.0f)
    {
    }
};
#pragma pack(pop)

namespace YAML {

    template<>
    struct convert<AtmosphereDto>
    {
        static Node encode(const AtmosphereDto& rhs)
        {
            Node n;

            n["hasAtmosphere"] = rhs.hasAtmosphere;
            n["environmentType"] = rhs.environmentType;
            n["sunEulerDegrees"] = rhs.sunEulerDegrees;
            n["sunBaseIntensity"] = rhs.sunBaseIntensity;

            n["rayleighScattering"] = rhs.rayleighScattering;
            n["rayleighScaleHeight"] = rhs.rayleighScaleHeight;

            n["mieScattering"] = rhs.mieScattering;
            n["mieScaleHeight"] = rhs.mieScaleHeight;
            n["mieAnisotropy"] = rhs.mieAnisotropy;

            n["ozoneAbsorption"] = rhs.ozoneAbsorption;
            n["ozoneDensity"] = rhs.ozoneDensity;

            n["planetRadius"] = rhs.planetRadius;
            n["atmosphereRadius"] = rhs.atmosphereRadius;

            n["sunColor"] = rhs.sunColor;
            n["sunIntensityMultiplier"] = rhs.sunIntensityMultiplier;

            n["exposure"] = rhs.exposure;
            n["skyTint"] = rhs.skyTint;
            n["horizonSoftness"] = rhs.horizonSoftness;
            n["multiScatteringFactor"] = rhs.multiScatteringFactor;

            n["sunDiskSize"] = rhs.sunDiskSize;
            n["sunDiskIntensity"] = rhs.sunDiskIntensity;
            n["sunGlow"] = rhs.sunGlow;

            return n;
        }

        static bool decode(const Node& n, AtmosphereDto& rhs)
        {
            if (!n || !n.IsMap()) return false;

            AtmosphereDto def; // fallback

            rhs.hasAtmosphere = n["hasAtmosphere"] ? n["hasAtmosphere"].as<bool>() : def.hasAtmosphere;
            rhs.environmentType = n["environmentType"] ? n["environmentType"].as<int>() : def.environmentType;

            rhs.sunEulerDegrees = n["sunEulerDegrees"] ? n["sunEulerDegrees"].as<glm::vec3>() : def.sunEulerDegrees;
            rhs.sunBaseIntensity = n["sunBaseIntensity"] ? n["sunBaseIntensity"].as<float>() : def.sunBaseIntensity;

            rhs.rayleighScattering = n["rayleighScattering"] ? n["rayleighScattering"].as<glm::vec3>() : def.rayleighScattering;
            rhs.rayleighScaleHeight = n["rayleighScaleHeight"] ? n["rayleighScaleHeight"].as<float>() : def.rayleighScaleHeight;

            rhs.mieScattering = n["mieScattering"] ? n["mieScattering"].as<glm::vec3>() : def.mieScattering;
            rhs.mieScaleHeight = n["mieScaleHeight"] ? n["mieScaleHeight"].as<float>() : def.mieScaleHeight;
            rhs.mieAnisotropy = n["mieAnisotropy"] ? n["mieAnisotropy"].as<float>() : def.mieAnisotropy;

            rhs.ozoneAbsorption = n["ozoneAbsorption"] ? n["ozoneAbsorption"].as<glm::vec3>() : def.ozoneAbsorption;
            rhs.ozoneDensity = n["ozoneDensity"] ? n["ozoneDensity"].as<float>() : def.ozoneDensity;

            rhs.planetRadius = n["planetRadius"] ? n["planetRadius"].as<float>() : def.planetRadius;
            rhs.atmosphereRadius = n["atmosphereRadius"] ? n["atmosphereRadius"].as<float>() : def.atmosphereRadius;

            rhs.sunColor = n["sunColor"] ? n["sunColor"].as<glm::vec3>() : def.sunColor;
            rhs.sunIntensityMultiplier = n["sunIntensityMultiplier"] ? n["sunIntensityMultiplier"].as<float>() : def.sunIntensityMultiplier;

            rhs.exposure = n["exposure"] ? n["exposure"].as<float>() : def.exposure;
            rhs.skyTint = n["skyTint"] ? n["skyTint"].as<float>() : def.skyTint;
            rhs.horizonSoftness = n["horizonSoftness"] ? n["horizonSoftness"].as<float>() : def.horizonSoftness;
            rhs.multiScatteringFactor = n["multiScatteringFactor"] ? n["multiScatteringFactor"].as<float>() : def.multiScatteringFactor;

            rhs.sunDiskSize = n["sunDiskSize"] ? n["sunDiskSize"].as<float>() : def.sunDiskSize;
            rhs.sunDiskIntensity = n["sunDiskIntensity"] ? n["sunDiskIntensity"].as<float>() : def.sunDiskIntensity;
            rhs.sunGlow = n["sunGlow"] ? n["sunGlow"].as<float>() : def.sunGlow;

            return true;
        }
    };

}

// ---- Helpers ----
inline YAML::Node SerializeAtmosphere(const AtmosphereDto& a)
{
    return YAML::convert<AtmosphereDto>::encode(a);
}

inline bool DeserializeAtmosphere(const YAML::Node& node, AtmosphereDto& out)
{
    if (!node) return false;
    out = node.as<AtmosphereDto>();
    return true;
}



namespace QE
{
    using ::AtmosphereDto;
} // namespace QE
// QE namespace aliases
#endif // ATMOSPHERE_DTO_H
