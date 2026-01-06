#ifndef QE_PBR_LIGHTS_GLSL
#define QE_PBR_LIGHTS_GLSL

#include "../QEShadows.glsl"
#include "QEPBR.glsl"

vec3 ComputePointLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness,
    samplerCube shadowCube
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    vec3 radiance = light.diffuse; // diffuse = color/intensidad de luz
    vec3 brdf = BRDF_CookTorrance(N, V, L, baseColor, metallic, roughness);

    float visibility = GetCMVisibility(shadowCube, -lightVec, dist);

    return brdf * radiance * attenuation * visibility;
}

vec3 ComputeDirectionalLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness,
    sampler2DArray shadowMap,
    vec4 splitsEnd,
    float viewDepth,
    mat4 vp0,
    mat4 vp1,
    uint c0,
    uint c1
){
    vec3 L = normalize(-light.direction);

    vec3 radiance = light.diffuse;
    vec3 brdf = BRDF_CookTorrance(N, V, L, baseColor, metallic, roughness);

    float visibility = GetCSMVisibility(
        shadowMap, splitsEnd, viewDepth, fragPosWorld,
        vp0, vp1, c0, c1
    );

    return brdf * radiance * visibility;
}

vec3 ComputeSpotLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    float theta = dot(L, normalize(-light.direction));
    float eps = max(light.cutoff - light.outerCutoff, 1e-6);
    float intensity = clamp((theta - light.outerCutoff) / eps, 0.0, 1.0);

    vec3 radiance = light.diffuse;
    vec3 brdf = BRDF_CookTorrance(N, V, L, baseColor, metallic, roughness);

    return brdf * radiance * (intensity * attenuation);
}

#endif // QE_PBR_LIGHTS_GLSL
