#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout(set = 0, binding = 0) uniform LightCameraUniform
{
	mat4 viewproj;
} lightCameraData;

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

void main() 
{
    gl_Position = lightCameraData.viewproj * constants.model * inPosition;
}
