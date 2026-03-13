#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../Includes/QECommon.glsl"

layout(set = 0, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

layout(location = 0) in vec4 inPosition;

void main() 
{
    gl_Position = cameraData.viewproj * constants.model * inPosition;
}
