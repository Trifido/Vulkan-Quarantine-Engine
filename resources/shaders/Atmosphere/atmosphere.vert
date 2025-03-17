#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out VS_OUT 
{
    vec2 TexCoords;
} QE_out;

void main() {
    QE_out.TexCoords = inPosition.xy * 0.5 + 0.5; // Convertir de [-1,1] a [0,1]
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
}
