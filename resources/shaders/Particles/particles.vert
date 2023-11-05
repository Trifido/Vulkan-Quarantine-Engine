#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 texCoord;

struct Particle 
{
	vec3    position;
    float   lifeTime;
    vec4    color;
	vec3    velocity;
    float   seed;
};

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec3 position;
} cameraData;

layout(std140, binding = 1) buffer ParticleSSBO 
{
   Particle particles[ ];
};

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

vec2 texCoords[6] =
{
    vec2(0,0),
    vec2(1,0),
    vec2(1,1),
    vec2(0,0),
    vec2(1,1),
    vec2(0,1)
};

vec2 quadCoords[6] = 
{
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5),
};

void main() 
{
    uint particleIndex = gl_VertexIndex / 6;
    uint vertexInQuad = gl_VertexIndex % 6;

    Particle pa = particles[particleIndex];
    vec3 center = pa.position;
    vec2 quad = quadCoords[vertexInQuad];
    vec2 uv = texCoords[vertexInQuad];

    vec3 cameraRight = vec3(cameraData.view[0][0], cameraData.view[1][0], cameraData.view[2][0]);
    vec3 cameraUp = vec3(cameraData.view[0][1], cameraData.view[1][1], cameraData.view[2][1]);

    vec3 vertexCoord = center + cameraRight * quad.x - cameraUp * quad.y;

    vec4 position = constants.model * vec4(vertexCoord, 1.0);
    gl_Position = cameraData.viewproj * position;
    
    fragColor = pa.color;
    texCoord = uv;
}
