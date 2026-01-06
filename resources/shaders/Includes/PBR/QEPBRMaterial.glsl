#ifndef QE_PBR_MATERIAL_GLSL
#define QE_PBR_MATERIAL_GLSL

#define QE_NUM_TEX 8
#define QE_SLOT_BASECOLOR 0u
#define QE_SLOT_NORMAL    1u
#define QE_SLOT_METALLIC  2u
#define QE_SLOT_ROUGHNESS 3u
#define QE_SLOT_AO        4u
#define QE_SLOT_EMISSIVE  5u
#define QE_SLOT_HEIGHT    6u
#define QE_SLOT_SPECULAR  7u

struct QEPBRMaterialData
{
    vec4 Diffuse;
    vec4 Ambient;
    vec4 Specular;
    vec4 Emissive;

    vec4 Transparent;
    vec4 Reflective;

    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
    int idxMetallic;
    int idxRoughness;
    int idxAO;

    float Metallic;
    float Roughness;
    float AO;

    float Opacity;
    float BumpScaling;

    float Reflectivity;
    float Refractivity;
    float Shininess;
    float Shininess_Strength;

    uint texMask;
};

bool QE_HasTex(uint mask, uint slot)
{
    return (mask & (1u << slot)) != 0u;
}

float QE_Saturate(float x) { return clamp(x, 0.0, 1.0); }

// --- Sampling helpers ---
vec3 QE_GetBaseColor(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 c = mat.Diffuse.rgb;
    if (QE_HasTex(mat.texMask, QE_SLOT_BASECOLOR))
        c = texture(texSampler[nonuniformEXT(mat.idxDiffuse)], uv).rgb;

    return c;
}

float QE_GetMetallic(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float m = mat.Metallic;
    if (QE_HasTex(mat.texMask, QE_SLOT_METALLIC))
        m = texture(texSampler[nonuniformEXT(mat.idxMetallic)], uv).r;
    return QE_Saturate(m);
}

float QE_GetRoughness(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float r = mat.Roughness;
    if (QE_HasTex(mat.texMask, QE_SLOT_ROUGHNESS))
        r = texture(texSampler[nonuniformEXT(mat.idxRoughness)], uv).r;

    // mínimo para estabilidad GGX
    return clamp(r, 0.045, 1.0);
}

float QE_GetAO(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float ao = mat.AO;
    if (QE_HasTex(mat.texMask, QE_SLOT_AO))
        ao = texture(texSampler[nonuniformEXT(mat.idxAO)], uv).r;
    return QE_Saturate(ao);
}

vec3 QE_GetEmissiveColor(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 e = mat.Emissive.rgb;
    if (QE_HasTex(mat.texMask, QE_SLOT_EMISSIVE))
        e = texture(texSampler[nonuniformEXT(mat.idxEmissive)], uv).rgb;
    return e;
}

vec3 QE_GetNormal(QEPBRMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv, vec3 normalWS, mat3 TBN)
{
    vec3 N = normalize(normalWS);

    if (QE_HasTex(mat.texMask, QE_SLOT_NORMAL))
    {
        vec3 nTS = texture(texSampler[nonuniformEXT(mat.idxNormal)], uv).xyz;
        nTS = nTS * 2.0 - 1.0;
        nTS.xy *= mat.BumpScaling;
        nTS = normalize(nTS);

        N = normalize(TBN * nTS);
    }

    return N;
}

vec3 QE_ComputeAmbientPBR(QEPBRMaterialData mat, vec3 baseColor, float metallic, float ao)
{
    vec3 kD = (1.0 - metallic) * baseColor;
    return kD * mat.Ambient.rgb * ao;
}

#endif // QE_PBR_MATERIAL_GLSL
