#ifndef QE_PBR_LIGHTS_GLSL
#define QE_PBR_LIGHTS_GLSL

#include "../QEShadows.glsl"
#include "QEPBR.glsl"

vec3 ComputePointLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N_base,
    vec3 N_coat,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness,
    float clearcoat,
    float clearcoatRoughness,
    samplerCube shadowCube
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
    vec3 radiance = light.diffuse;

    float visibility = GetCMVisibility(shadowCube, -lightVec, dist);

    vec3 brdfBase = BRDF_CookTorrance(N_base, V, L, baseColor, metallic, roughness);
    vec3 brdfCoat = BRDF_Clearcoat(N_coat, V, L, clearcoat, clearcoatRoughness);

    float coatAtten = 1.0 - 0.25 * saturate(clearcoat);
    vec3 lighting = (brdfBase * coatAtten + brdfCoat);

    return lighting * radiance * attenuation * visibility;
}

vec3 ComputeDirectionalLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N_base,
    vec3 N_coat,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness,
    float clearcoat,
    float clearcoatRoughness,
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

    float visibility = GetCSMVisibility(
        shadowMap, splitsEnd, viewDepth, fragPosWorld,
        vp0, vp1, c0, c1
    );

    vec3 brdfBase = BRDF_CookTorrance(N_base, V, L, baseColor, metallic, roughness);
    vec3 brdfCoat = BRDF_Clearcoat(N_coat, V, L, clearcoat, clearcoatRoughness);

    float coatAtten = 1.0 - 0.25 * saturate(clearcoat);
    vec3 lighting = (brdfBase * coatAtten + brdfCoat);

    return lighting * radiance * visibility;
}


vec3 ComputeSpotLightPBR(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N_base,
    vec3 N_coat,
    vec3 V,
    vec3 baseColor,
    float metallic,
    float roughness,
    float clearcoat,
    float clearcoatRoughness
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    float theta = dot(L, normalize(-light.direction));
    float eps = max(light.cutoff - light.outerCutoff, 1e-6);
    float intensity = clamp((theta - light.outerCutoff) / eps, 0.0, 1.0);

    vec3 radiance = light.diffuse;

    vec3 brdfBase = BRDF_CookTorrance(N_base, V, L, baseColor, metallic, roughness);
    vec3 brdfCoat = BRDF_Clearcoat(N_coat, V, L, clearcoat, clearcoatRoughness);

    float coatAtten = 1.0 - 0.25 * saturate(clearcoat);
    vec3 lighting = (brdfBase * coatAtten + brdfCoat);

    return lighting * radiance * (intensity * attenuation);
}


#endif // QE_PBR_LIGHTS_GLSL
