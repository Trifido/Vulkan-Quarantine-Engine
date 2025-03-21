#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out VS_OUT 
{
    vec2 TexCoords;
} QE_out;

void main() {
    // Obtener el ID del vértice
    int vertexID = gl_VertexIndex;

    // Calcular coordenadas UV en el rango [0,1]
    QE_out.TexCoords = vec2((vertexID << 1) & 2, vertexID & 2);

    // Convertir UVs a Clip Space
    gl_Position = vec4(QE_out.TexCoords * 2.0 - 1.0, 0.0, 1.0);

    // CORRECCIÓN PARA VULKAN: invertir Y
    gl_Position.y = -gl_Position.y;
}