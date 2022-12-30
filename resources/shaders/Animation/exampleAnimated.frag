#version 450
#extension GL_ARB_separate_shader_objects : enable

const int numberOfLights = 8;

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec3 position;
} cameraData;

layout(set = 0, binding = 1) uniform UniformMaterial {
    float shininess;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
} uboMaterial;

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

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
	LightData lights[8];
} uboLight;

layout(set = 0, binding = 4) uniform sampler2D texSampler[6];


void main()
{
    vec3 result = vec3(texture(texSampler[uboMaterial.idxDiffuse], fs_in.TexCoords));
    outColor = vec4(result, 1.0);
}