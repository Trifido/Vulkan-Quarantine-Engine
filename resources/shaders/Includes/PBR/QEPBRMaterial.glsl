#ifndef QE_PBR_MATERIAL_GLSL
#define QE_PBR_MATERIAL_GLSL

#define QE_NUM_TEX 8

// Si lo usas como UBO real, revisa std140/padding en C++ y GLSL.
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
};

// --- Sampling helpers ---
vec3 QE_GetBaseColor(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 c = mat.Diffuse.rgb;
    if (mat.idxDiffuse > -1)
        c = texture(texSampler[nonuniformEXT(mat.idxDiffuse)], uv).rgb;

    return c;
}

float QE_GetMetallic( QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float m = mat.Metallic;
    if (mat.idxMetallic > -1)
        m = texture(texSampler[nonuniformEXT(mat.idxMetallic)], uv).r;
    return QE_Saturate(m);
}

float QE_GetRoughness(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float r = mat.Roughness;
    if (mat.idxRoughness > -1)
        r = texture(texSampler[nonuniformEXT(mat.idxRoughness)], uv).r;

    // mínimo para estabilidad GGX
    return clamp(r, 0.045, 1.0);
}

float QE_GetAO(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    float ao = mat.AO;
    if (mat.idxAO > -1)
        ao = texture(texSampler[nonuniformEXT(mat.idxAO)], uv).r;
    return QE_Saturate(ao);
}

vec3 QE_GetEmissiveColor(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 e = mat.Emissive.rgb;
    if (mat.idxEmissive > -1)
        e = texture(texSampler[nonuniformEXT(mat.idxEmissive)], uv).rgb;
    return e;
}

vec3 QE_GetNormal(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv, vec3 normalWS, mat3 TBN)
{
    vec3 N = normalize(normalWS);

    if (mat.idxNormal > -1)
    {
        vec3 nTS = texture(texSampler[nonuniformEXT(mat.idxNormal)], uv).xyz;
        nTS = nTS * 2.0 - 1.0;
        nTS.xy *= mat.BumpScaling;
        nTS = normalize(nTS);

        N = normalize(TBN * nTS);
    }

    return N;
}

vec3 QE_ComputeAmbientPBR(QEMaterialData mat, vec3 baseColor, float metallic, float ao)
{
    vec3 kD = (1.0 - metallic) * baseColor;
    return kD * mat.Ambient.rgb * ao;
}

#endif // QE_PBR_MATERIAL_GLSL
