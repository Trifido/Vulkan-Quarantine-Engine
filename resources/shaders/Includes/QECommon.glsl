#ifndef QE_COMMON_GLSL
#define QE_COMMON_GLSL

struct QELightData
{
    vec3 position;
    uint lightType;
    vec3 diffuse;
    float constant;
    vec3 specular;
    float linear;
    vec3 direction;
    float quadratic; 
    float cutoff;
    float outerCutoff;
    float radius;
    uint idxShadowMap;
};

struct QECameraData
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
};

#endif