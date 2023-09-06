#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec3 position;
} cameraData;

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

struct LightData {
    vec4 position;
    vec3 diffuse;
    float constant;
    vec3 specular;
    float linear;
    vec3 spotDirection;
    float quadratic; 
    float spotCutoff;
    float spotExponent;
};

layout(set = 0, binding = 1) uniform UniformManagerLight
{
    int numLights;
	LightData lights[8];
} uboLight;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;

layout(location = 0) out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 TangentLightPos[8];
    vec2 TexCoords;
} vs_out;

void main() 
{
    vs_out.FragPos = vec3(constants.model * vec4(inPosition, 1.0));
    vs_out.TexCoords = inTexCoord;
    vs_out.Normal = inNormal;

    vec3 T = normalize(inTangent);
    vec3 N = normalize(inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    
    mat3 TBN = transpose(mat3(T, B, N));   

    vs_out.TangentViewPos = TBN * cameraData.position;
    vs_out.TangentFragPos = TBN * vs_out.FragPos; 

    for(int i = 0; i < uboLight.numLights; i++)
    {
        vs_out.TangentLightPos[i] = TBN * vec3(uboLight.lights[i].position);
    }

    gl_Position = cameraData.viewproj * vec4(vs_out.FragPos, 1.0);
}
