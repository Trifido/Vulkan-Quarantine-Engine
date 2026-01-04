#ifndef QE_PBR_GLSL
#define QE_PBR_GLSL

const float PI = 3.14159265359;

// --- Helpers ---
float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3  saturate(vec3  x) { return clamp(x, vec3(0.0), vec3(1.0)); }

float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / max(PI * d * d, 1e-7);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    // Disney/UE4 style k
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / max(NdotX * (1.0 - k) + k, 1e-7);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    float ggxV = G_SchlickGGX(NdotV, roughness);
    float ggxL = G_SchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

vec3 F_Schlick(vec3 F0, float VdotH)
{
    // Schlick Fresnel
    float f = pow(1.0 - VdotH, 5.0);
    return F0 + (1.0 - F0) * f;
}

// Devuelve radiancia directa (sin sombras/atenuación) para una luz direccional ya evaluada
vec3 BRDF_CookTorrance(
    vec3 N, vec3 V, vec3 L,
    vec3 baseColor, float metallic, float roughness
){
    vec3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // Clamp roughness para estabilidad numérica
    roughness = clamp(roughness, 0.045, 1.0);

    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    vec3  F = F_Schlick(F0, VdotH);
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);

    vec3 numerator   = D * G * F;
    float denom      = max(4.0 * NdotV * NdotL, 1e-6);
    vec3 specular    = numerator / denom;

    // Conservación de energía
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 diffuse = kD * baseColor / PI;

    return (diffuse + specular) * NdotL;
}

#endif // QE_PBR_GLSL
