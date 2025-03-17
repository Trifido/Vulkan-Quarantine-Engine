#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT 
{
    vec2 TexCoords;
} QE_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

const float PI = 3.14159265359;

vec3 HosekWilkie(float cos_theta, float gamma, float cos_gamma)
{
	vec3 A = params[0];
	vec3 B = params[1];
	vec3 C = params[2];
	vec3 D = params[3];
	vec3 E = params[4];
	vec3 F = params[5];
	vec3 G = params[6];
	vec3 H = params[7];
	vec3 I = params[8];
	vec3 Z = params[9];
	vec3 chi = (1 + cos_gamma * cos_gamma) / pow(1 + H * H - 2 * cos_gamma * H, vec3(1.5));
    return (1 + A * exp(B / (cos_theta + 0.01))) * (C + D * exp(E * gamma) + F * (cos_gamma * cos_gamma) + G * chi + I * sqrt(cos_theta));
}

void main()
{
    vec2 ndc = QE_in.TexCoords * 2.0 - 1.0; // Convertimos de [0,1] a [-1,1]
    vec4 clipPos = vec4(ndc, -1.0, 1.0);
    vec4 viewPos = inverse(cameraData.proj) * clipPos;
    viewPos = vec4(viewPos.xy, -1.0, 0.0); // Corregir w=0 para dirección
    vec3 viewDir = normalize((inverse(cameraData.view) * viewPos).xyz); // Convertir a espacio mundial

    vec3 sunDir = vec3(0.0, 1.0, 0.0);
    sunDir = normalize(sunDir);

    float cos_theta = clamp(viewDir.y, 0, 1);
	float cos_gamma = dot(viewDir, sunDir);
	float gamma = acos(cos_gamma);

    vec3 Z = params[9];
	vec3 R = Z * HosekWilkie(cos_theta, gamma, cos_gamma);
	if (cos_gamma > 0) 
    {
		// Only positive values of dot product, so we don't end up creating two
		// spots of light 180 degrees apart
		R = R + pow(vec3(cos_gamma), vec3(256)) * 0.5;
	}
	outColor = vec4(R, 1.0);
}