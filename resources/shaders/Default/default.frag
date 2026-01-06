#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#include "../Includes/QECommon.glsl"
#include "../Includes/PBR/QEPBRLights.glsl"
#include "../Includes/PBR/QEPBRMaterial.glsl"

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2
#define AREA_LIGHT 3
#define SUN_LIGHT 4

#define NUM_BINS 16.0
#define BIN_WIDTH ( 1.0 / NUM_BINS )
#define MAX_NUM_LIGHTS 64
#define NUM_WORDS ( ( MAX_NUM_LIGHTS + 31 ) / 32 )
#define FAR_PLANE 500.0

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 ViewPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(set = 0, binding = 1, std140) uniform UniformMaterial
{
    QEPBRMaterialData uboMaterial;
};

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
} uboLight;

layout(set = 0, binding = 3) readonly buffer LightSSBO 
{
   QELightData lights[];
};

layout(set = 0, binding = 4) readonly buffer LightIndices 
{
    uint light_indices[];
};

layout(set = 0, binding = 5) readonly buffer ZBins 
{
    uint bins[];
};

layout(set = 0, binding = 6) readonly buffer Tiles 
{
    uint tiles[];
};

layout(set = 0, binding = 7) uniform sampler2D texSampler[QE_NUM_TEX];

layout(set = 0, binding = 8) uniform ScreenData 
{
    uvec2 pix_tile_size;
} screenData;

layout(set = 1, binding = 0) uniform samplerCube QE_PointShadowCubemaps[10];

layout(set = 2, binding = 0) uniform sampler2DArray QE_DirectionalShadowmaps[10];

layout (set = 2, binding = 1) uniform UniformCSM
{
    vec4 Splits[10];
} QE_Cascade;

layout (set = 2, binding = 2) readonly buffer cascadeViewProjs
{
    mat4 QE_CascadeViewProj[];
};

void main()
{
    vec4 pos_camera_space = cameraData.view * vec4(fs_in.FragPos, 1.0);
    float z_far = FAR_PLANE;
    float z_near = 0.1;
    float linear_d = (-pos_camera_space.z - z_near) / (z_far - z_near);
    linear_d = clamp(linear_d, 0.0, 0.999999);
    int bin_index = int( linear_d / BIN_WIDTH );
    bin_index = clamp(bin_index, 0, int(NUM_BINS) - 1);
    uint bin_value = bins[ bin_index ];

    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = ( bin_value >> 16 ) & 0xFFFF;

    uvec2 tile = uvec2(gl_FragCoord.xy / screenData.pix_tile_size);
    uint stride = uint( NUM_WORDS ) * screenData.pix_tile_size.x;
    uint address = tile.y * stride + tile.x;

    vec3 fragPos = fs_in.FragPos;
    vec3 V = normalize(cameraData.position.xyz - fragPos);
    float viewDepth = -fs_in.ViewPos.z;
    float shininess = uboMaterial.Shininess;

    vec3 normal = QE_GetNormal(uboMaterial, texSampler, fs_in.TexCoords, fs_in.Normal, fs_in.TBN);
    vec3 albedoColor = QE_GetBaseColor(uboMaterial, texSampler, fs_in.TexCoords);
    vec3 emissiveColor = QE_GetEmissiveColor(uboMaterial, texSampler, fs_in.TexCoords);

    float roughness = QE_GetRoughness(uboMaterial, texSampler, fs_in.TexCoords);
    float metallic = QE_GetMetallic(uboMaterial, texSampler, fs_in.TexCoords);
    float ao = QE_GetAO(uboMaterial, texSampler, fs_in.TexCoords);
    vec3 result = QE_ComputeAmbientPBR(uboMaterial, albedoColor, metallic, ao);

    vec3 resultPoint = vec3(0.0);
    vec3 resultDir = vec3(0.0);
    vec3 resultSpot = vec3(0.0);
    
    if (min_light_id != uboLight.numLights + 1)
    {
        for (uint light_id = min_light_id; light_id <= max_light_id; ++light_id) 
        {
            uint word_id = light_id / 32;
            uint bit_id = light_id % 32;

            if ((tiles[ address + word_id ] & (1 << bit_id)) != 0) 
            {
                uint gli = light_indices[light_id];

                if (lights[gli].lightType == POINT_LIGHT)
                {
                    resultPoint += ComputePointLightPBR(
                        lights[gli], fragPos, normal, V,
                        albedoColor, metallic, roughness,
                        QE_PointShadowCubemaps[nonuniformEXT(lights[gli].idxShadowMap)]
                    );
                }
                else if (lights[gli].lightType == DIRECTIONAL_LIGHT || lights[gli].lightType == SUN_LIGHT)
                {
                    uint si = lights[gli].idxShadowMap;

                    vec4 splits = QE_Cascade.Splits[si];

                    uint c0 = 0u;
                    if (viewDepth > splits.x) c0 = 1u;
                    if (viewDepth > splits.y) c0 = 2u;
                    if (viewDepth > splits.z) c0 = 3u;
                    uint c1 = min(c0 + 1u, uint(CSM_COUNT - 1));

                    uint vp0i = CSM_COUNT * si + c0;
                    uint vp1i = CSM_COUNT * si + c1;

                    mat4 vp0 = QE_CascadeViewProj[vp0i];
                    mat4 vp1 = QE_CascadeViewProj[vp1i];

                    resultDir += ComputeDirectionalLightPBR(
                        lights[gli], fragPos, normal, V,
                        albedoColor, metallic, roughness,
                        QE_DirectionalShadowmaps[nonuniformEXT(si)],
                        splits, viewDepth,
                        vp0, vp1, c0, c1
                    );
                }
                else
                {
                    resultSpot += ComputeSpotLightPBR(
                        lights[gli], fragPos, normal, V,
                        albedoColor, metallic, roughness
                    );
                }
            }
        }  
    }

    result += resultPoint + resultDir + resultSpot;
    outColor = vec4(result, uboMaterial.Opacity);
}