#include "CapsuleMesh.h"
#include <MeshImporter.h>

CapsuleMesh::CapsuleMesh() : PrimitiveMesh(PRIMITIVE_TYPE::CAPSULE_TYPE)
{
    this->type = PRIMITIVE_TYPE::CAPSULE_TYPE;
    this->capsuleRadius = 0.5f;
    this->capsuleHeight = 1.0f;
    this->capsuleSegments = 16;
    this->capsuleRings = 8;
}

void CapsuleMesh::InitializeMesh()
{
    this->GenerateCapsuleMesh();

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);

    this->numVertices = (uint32_t)this->indices.size();
    this->numFaces = this->numVertices / 3;

    this->createVertexBuffer();
    this->createIndexBuffer();

    glm::vec3 minValue = glm::vec3(-this->capsuleRadius, -this->capsuleHeight / 2.0f, -this->capsuleRadius);
    glm::vec3 maxValue = glm::vec3(this->capsuleRadius, this->capsuleHeight / 2.0f, this->capsuleRadius);
    this->aabbData = std::pair<glm::vec3, glm::vec3>(minValue, maxValue);
}

void CapsuleMesh::GenerateCapsuleMesh()
{
    const float PI = 3.14159265358979323846f;

    // ---- 1. CILINDRO CENTRAL ----
    for (int i = 0; i <= this->capsuleRings; ++i)
    {
        float y = -this->capsuleHeight / 2.0f + ((float)i / this->capsuleRings) * this->capsuleHeight;
        for (int j = 0; j <= this->capsuleSegments; ++j)
        {
            float theta = 2.0f * PI * ((float)j / this->capsuleSegments);
            float x = this->capsuleRadius * cos(theta);
            float z = this->capsuleRadius * sin(theta);

            Vertex vertex = {};
            vertex.pos = glm::vec4(x, y, z, 1.0f);
            this->vertices.push_back(vertex);
        }
    }

    int cylinderVerts = (this->capsuleRings + 1) * (this->capsuleSegments + 1);

    for (int i = 0; i < this->capsuleRings; ++i)
    {
        for (int j = 0; j < this->capsuleSegments; ++j)
        {
            int a = i * (this->capsuleSegments + 1) + j;
            int b = a + this->capsuleSegments + 1;

            this->indices.push_back(a);
            this->indices.push_back(b);
            this->indices.push_back(a + 1);

            this->indices.push_back(b);
            this->indices.push_back(b + 1);
            this->indices.push_back(a + 1);
        }
    }

    // ---- 2. HEMISFERIOS ----
    // Superior
    int baseIndex = (int)this->vertices.size();
    for (int i = 0; i <= this->capsuleRings; ++i)
    {
        float phi = (PI / 2.0f) * ((float)i / this->capsuleRings);
        float y = this->capsuleRadius * sin(phi);
        float r = this->capsuleRadius * cos(phi);

        for (int j = 0; j <= this->capsuleSegments; ++j)
        {
            float theta = 2.0f * PI * ((float)j / this->capsuleSegments);
            float x = r * cos(theta);
            float z = r * sin(theta);

            Vertex vertex = {};
            vertex.pos = glm::vec4(glm::vec3(x, y + this->capsuleHeight / 2.0f, z), 1.0f);
            this->vertices.push_back(vertex);
        }
    }

    int hemisphereRings = this->capsuleRings;
    for (int i = 0; i < hemisphereRings; ++i)
    {
        for (int j = 0; j < this->capsuleSegments; ++j)
        {
            int a = baseIndex + i * (this->capsuleSegments + 1) + j;
            int b = a + this->capsuleSegments + 1;

            this->indices.push_back(a);
            this->indices.push_back(b);
            this->indices.push_back(a + 1);

            this->indices.push_back(b);
            this->indices.push_back(b + 1);
            this->indices.push_back(a + 1);
        }
    }

    // Inferior (espejado)
    baseIndex = (int)this->vertices.size();
    for (int i = 0; i <= this->capsuleRings; ++i)
    {
        float phi = (PI / 2.0f) * ((float)i / this->capsuleRings);
        float y = -this->capsuleRadius * sin(phi);
        float r = this->capsuleRadius * cos(phi);

        for (int j = 0; j <= this->capsuleSegments; ++j)
        {
            float theta = 2.0f * PI * ((float)j / this->capsuleSegments);
            float x = r * cos(theta);
            float z = r * sin(theta);

            Vertex vertex = {};
            vertex.pos = glm::vec4(glm::vec3(x, y - this->capsuleHeight / 2.0f, z), 1.0f);
            this->vertices.push_back(vertex);
        }
    }

    for (int i = 0; i < hemisphereRings; ++i)
    {
        for (int j = 0; j < this->capsuleSegments; ++j)
        {
            int a = baseIndex + i * (this->capsuleSegments + 1) + j;
            int b = a + this->capsuleSegments + 1;

            this->indices.push_back(a);
            this->indices.push_back(a + 1);
            this->indices.push_back(b);

            this->indices.push_back(b);
            this->indices.push_back(a + 1);
            this->indices.push_back(b + 1);
        }
    }
}
