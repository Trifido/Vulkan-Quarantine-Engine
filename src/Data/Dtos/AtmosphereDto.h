#pragma once
#ifndef ATMOSPHERE_DTO_H
#define ATMOSPHERE_DTO_H

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <glm_yaml_conversions.h>

#pragma pack(1)
struct AtmosphereDto
{
    bool hasAtmosphere;
    int environmentType;
    glm::vec3 sunDirection;
    float sunIntensity;

    AtmosphereDto()
        : hasAtmosphere(true),
        environmentType(2),
        sunDirection{ 0.0f, -0.1f, 0.1f },
        sunIntensity(100.0f)
    {
    }

    AtmosphereDto(bool hasAtmosphere, int environmentType, glm::vec3 sunDirection, float sunIntensity)
        : hasAtmosphere(hasAtmosphere),
        environmentType(environmentType),
        sunDirection(sunDirection),
        sunIntensity(sunIntensity)
    {
    }
};
#pragma pack()


namespace YAML {
    template<> struct convert<AtmosphereDto> {
        static Node encode(const AtmosphereDto& rhs) {
            Node n;
            n["hasAtmosphere"] = rhs.hasAtmosphere;
            n["environmentType"] = rhs.environmentType;
            n["sunDirection"] = rhs.sunDirection;
            n["sunIntensity"] = rhs.sunIntensity;
            return n;
        }

        static bool decode(const Node& n, AtmosphereDto& rhs) {
            if (!n || !n.IsMap()) return false;
            AtmosphereDto def;

            rhs.hasAtmosphere = n["hasAtmosphere"] ? n["hasAtmosphere"].as<bool>() : def.hasAtmosphere;
            rhs.environmentType = n["environmentType"] ? n["environmentType"].as<int>() : def.environmentType;
            rhs.sunDirection = n["sunDirection"] ? n["sunDirection"].as<glm::vec3>() : def.sunDirection;
            rhs.sunIntensity = n["sunIntensity"] ? n["sunIntensity"].as<float>() : def.sunIntensity;
            return true;
        }
    };
}

// ---- Helpers ----
inline YAML::Node SerializeAtmosphere(const AtmosphereDto& a) {
    return YAML::convert<AtmosphereDto>::encode(a);
}

inline bool DeserializeAtmosphere(const YAML::Node& node, AtmosphereDto& out) {
    if (!node) return false;
    out = node.as<AtmosphereDto>();
    return true;
}

#endif // ATMOSPHERE_DTO_H
