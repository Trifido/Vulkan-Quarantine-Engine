#ifndef QE_BASIC_LIGHT_GLSL
#define QE_BASIC_LIGHT_GLSL

#include "QEShadows.glsl"

vec3 ComputeBlinnPhong(
    vec3 L, vec3 N, vec3 V,
    vec3 lightDiffuse, vec3 lightSpecular,
    vec3 albedo, vec3 specColor,
    float shininess
){
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * lightDiffuse * albedo;

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = spec * lightSpecular * specColor;

    return diffuse + specular;
}

vec3 ComputePointLight(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 albedo,
    vec3 specColor,
    vec3 emissive,
    float shininess,
    samplerCube shadowCube
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    vec3 direct = ComputeBlinnPhong(L, N, V, light.diffuse, light.specular, albedo, specColor, shininess) * attenuation;
    float visibility = GetCMVisibility(shadowCube, -lightVec, dist);

    return direct * visibility + emissive;
}

vec3 ComputeDirectionalLight(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 albedo,
    vec3 specColor,
    vec3 emissive,
    float shininess,
    sampler2DArray shadowMap,
    vec4 splitsEnd,
    float viewDepth,
    mat4 vp0,
    mat4 vp1,
    uint c0,
    uint c1
){
    vec3 L = normalize(-light.direction);

    vec3 direct = ComputeBlinnPhong(L, N, V, light.diffuse, light.specular, albedo, specColor, shininess);

    float visibility = GetCSMVisibility(
        shadowMap,
        splitsEnd,
        viewDepth,
        fragPosWorld,
        vp0,
        vp1,
        c0,
        c1
    );

    return direct * visibility + emissive;
}

vec3 ComputeSpotLight(
    QELightData light,
    vec3 fragPosWorld,
    vec3 N,
    vec3 V,
    vec3 albedo,
    vec3 specColor,
    vec3 emissive,
    float shininess
){
    vec3 lightVec = light.position - fragPosWorld;
    float dist = length(lightVec);
    vec3 L = lightVec / max(dist, 1e-6);

    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));

    float theta = dot(L, normalize(-light.direction));
    float eps = max(light.cutoff - light.outerCutoff, 1e-6);
    float intensity = clamp((theta - light.outerCutoff) / eps, 0.0, 1.0);

    vec3 direct = ComputeBlinnPhong(L, N, V, light.diffuse, light.specular, albedo, specColor, shininess);
    direct *= (intensity * attenuation);

    return direct + emissive;
}

#endif // QE_BASIC_LIGHT_GLSL