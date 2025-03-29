#version 450
#extension GL_ARB_separate_shader_objects : enable

void main() {
    // Obtener el ID del vértice
    int vertexID = gl_VertexIndex;

    // Calcular coordenadas UV en el rango [0,1]
    vec2 coords = vec2((vertexID << 1) & 2, vertexID & 2);

    // Convertir UVs a Clip Space
    gl_Position = vec4(coords * 2.0 - 1.0, 0.0, 1.0);
}