#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout (location = 0) out vec2 uv;
layout (location = 1) out vec2 out_camPos;

const vec3 pos[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
);

const int indices[6] = int[6](
	0, 3, 2, 0, 2, 1
);

float gridSize = 100.0;

void main()
{
    mat4 MVP = cameraData.viewproj;
	int idx = indices[gl_VertexIndex];
	vec3 position = pos[idx] * gridSize;
	
	position.x += cameraData.position.x;
	position.z += cameraData.position.z;

	out_camPos = cameraData.position.xz;

	gl_Position = MVP * vec4(position, 1.0);
	uv = position.xz;
}
