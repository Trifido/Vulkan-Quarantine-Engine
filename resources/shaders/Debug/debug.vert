#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../Includes/QECommon.glsl"

layout(set = 0, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

void main() 
{
    fragColor = inColor.rgb;
    gl_Position = cameraData.viewproj * inPosition;
}
