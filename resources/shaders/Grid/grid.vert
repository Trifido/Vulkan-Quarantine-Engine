#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 v_ndc;

const vec2 verts[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

void main()
{
    vec2 p = verts[gl_VertexIndex];
    v_ndc = p;
    gl_Position = vec4(p, 0.0, 1.0);
}