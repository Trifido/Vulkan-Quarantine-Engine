#ifndef QE_MATERIAL_GLSL
#define QE_MATERIAL_GLSL

#define QE_NUM_TEX 5

struct QEMaterialData
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

vec3 QE_GetSpecularColor(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 colorSpecular = mat.Specular.rgb;
    if(mat.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[nonuniformEXT(mat.idxSpecular)], uv));
    return colorSpecular;
}

vec3 QE_GetEmissiveColor(QEMaterialData mat, sampler2D texSampler[QE_NUM_TEX], vec2 uv)
{
    vec3 emissive = mat.Emissive.rgb;
    if(mat.idxEmissive > -1)
        emissive = vec3(texture(texSampler[nonuniformEXT(mat.idxEmissive)], uv));
    return emissive;
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

vec3 QE_ComputeAmbient(QEMaterialData mat)
{
    return mat.Diffuse.rgb * mat.Ambient.rgb;
}

#endif // QE_MATERIAL_GLSL