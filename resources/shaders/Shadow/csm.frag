#version 450
#extension GL_EXT_nonuniform_qualifier : require

#include "../Includes/QECommon.glsl"
#include "../Includes/PBR/QEPBRMaterial.glsl"

layout(location = 0) in vec2 inTexCoord;

layout(set = 1, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(set = 1, binding = 1, std140) uniform UniformMaterial
{
    QEPBRMaterialData uboMaterial;
};

layout(set = 1, binding = 2) uniform UniformManagerLight
{
    int numLights;
} uboLight;

layout(set = 1, binding = 3) readonly buffer LightSSBO
{
    QELightData lights[];
};

layout(set = 1, binding = 4) readonly buffer LightIndices
{
    uint light_indices[];
};

layout(set = 1, binding = 5) readonly buffer ZBins
{
    uint bins[];
};

layout(set = 1, binding = 6) readonly buffer Tiles
{
    uint tiles[];
};

layout(set = 1, binding = 7) uniform sampler2D texSampler[QE_NUM_TEX];

layout(set = 1, binding = 8) uniform ScreenData
{
    uvec2 tilePixelSize;
    uvec2 tileCount;
} screenData;

void main()
{
    vec4 base = QE_GetBaseColorAlpha(uboMaterial, texSampler, inTexCoord);
    if (QE_ShouldDiscardAlpha(uboMaterial, base))
        discard;
}
