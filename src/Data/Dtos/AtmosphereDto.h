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
    glm::vec3 sunEulerDegrees;
    float sunBaseIntensity;

    AtmosphereDto()
        : hasAtmosphere(true),
        environmentType(2),
        sunEulerDegrees{ -45.0f, 45.0f, 0.0f },
        sunBaseIntensity(100.0f)
    {
    }

    AtmosphereDto(bool hasAtmosphere, int environmentType, glm::vec3 sunEulerDegrees, float sunBaseIntensity)
        : hasAtmosphere(hasAtmosphere),
        environmentType(environmentType),
        sunEulerDegrees(sunEulerDegrees),
        sunBaseIntensity(sunBaseIntensity)
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
            n["sunEulerDegrees"] = rhs.sunEulerDegrees;
            n["sunBaseIntensity"] = rhs.sunBaseIntensity;
            return n;
        }

        static bool decode(const Node& n, AtmosphereDto& rhs) {
            if (!n || !n.IsMap()) return false;
            AtmosphereDto def;

            rhs.hasAtmosphere = n["hasAtmosphere"] ? n["hasAtmosphere"].as<bool>() : def.hasAtmosphere;
            rhs.environmentType = n["environmentType"] ? n["environmentType"].as<int>() : def.environmentType;
            rhs.sunEulerDegrees = n["sunEulerDegrees"] ? n["sunEulerDegrees"].as<glm::vec3>() : def.sunEulerDegrees;
            rhs.sunBaseIntensity = n["sunBaseIntensity"] ? n["sunBaseIntensity"].as<float>() : def.sunBaseIntensity;
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
