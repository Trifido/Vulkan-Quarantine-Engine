#ifndef QE_SHADOWS_GLSL
#define QE_SHADOWS_GLSL

#define SHADOW_OPACITY 0.5
#define CSM_COUNT 4

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

float ComputeCSMTextureProj(sampler2DArray shadowMap, vec4 shadowCoord, vec2 offset, uint cascadeIndex)
{
    if (shadowCoord.x < 0.0 || shadowCoord.x > 1.0 ||
        shadowCoord.y < 0.0 || shadowCoord.y > 1.0 ||
        shadowCoord.z < 0.0 || shadowCoord.z > 1.0)
        return 1.0;

    float baseBias = (cascadeIndex == 0u) ? 0.00010 :
                     (cascadeIndex == 1u) ? 0.00020 :
                     (cascadeIndex == 2u) ? 0.00035 : 0.00050;

    float closest = texture(shadowMap, vec3(shadowCoord.st + offset, cascadeIndex)).r;

    return (closest < shadowCoord.z - baseBias) ? SHADOW_OPACITY : 1.0;
}

float ComputeCSMFilterPCF(sampler2DArray shadowMap, vec4 shadowCoord, uint cascadeIndex)
{
    ivec3 dim = textureSize(shadowMap, 0);
    ivec2 texDim = dim.xy;

    float scale = (cascadeIndex == 0u) ? 0.85 :
                  (cascadeIndex == 1u) ? 1.10 : 1.60;

    float dx = scale / float(texDim.x);
    float dy = scale / float(texDim.y);

    int range = (cascadeIndex == 0u) ? 1 : 1;

    float sum = 0.0;
    int count = 0;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            sum += ComputeCSMTextureProj(shadowMap, shadowCoord, vec2(dx*x, dy*y), cascadeIndex);
            count++;
        }
    }

    return sum / float(count);
}

float GetCSMVisibility(
    sampler2DArray shadowMap,
    vec4 splitsEnd,
    float viewDepth,
    vec3 fragPosWorld,
    mat4 vp0,
    mat4 vp1,
    uint c0,
    uint c1
){
    float end0   = (c0 == 0u) ? splitsEnd.x : (c0 == 1u) ? splitsEnd.y : (c0 == 2u) ? splitsEnd.z : splitsEnd.w;
    float start0 = (c0 == 0u) ? 0.0        : (c0 == 1u) ? splitsEnd.x : (c0 == 2u) ? splitsEnd.y : splitsEnd.z;

    float range0 = max(end0 - start0, 1e-4);
    float fade   = (c0 == 0u) ? (0.03 * range0) : (0.06 * range0);
    float t      = clamp((viewDepth - (end0 - fade)) / max(fade, 1e-4), 0.0, 1.0);

    vec4 sc0 = (biasMat * vp0) * vec4(fragPosWorld, 1.0);
    vec4 sc1 = (biasMat * vp1) * vec4(fragPosWorld, 1.0);

    float sh0 = ComputeCSMFilterPCF(shadowMap, sc0 / sc0.w, c0);
    float sh1 = ComputeCSMFilterPCF(shadowMap, sc1 / sc1.w, c1);

    return mix(sh0, sh1, t);
}

float GetCMVisibility(samplerCube cubeMap, vec3 lightVec, float currentDepth)
{
    const int samples = 20;
    float bias = 0.01;
    float texelAngular = 1.0 / float(textureSize(cubeMap, 0).x);
    float diskRadius = texelAngular * currentDepth * 2.0;
    float litCount = 0.0;

    for (int i = 0; i < samples; ++i)
    {
        vec3 sampleDir = lightVec + sampleOffsetDirections[i] * diskRadius;
        float closestDepth = texture(cubeMap, sampleDir).r;
        litCount += (currentDepth <= closestDepth + bias) ? 1.0 : 0.0;
    }

    float visibility = litCount / float(samples);
    return mix(SHADOW_OPACITY, 1.0, visibility);
}

#endif