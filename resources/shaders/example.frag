#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UniformMaterial {
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
    int idxBump;
} uboMaterial;

layout(binding = 1) uniform sampler2D texSampler[6];

void main() {
    outColor = texture(texSampler[uboMaterial.idxDiffuse], fragTexCoord);
}
