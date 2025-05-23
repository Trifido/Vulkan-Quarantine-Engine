#pragma once

#ifndef IQE_MESH_GENERATOR_H
#define IQE_MESH_GENERATOR_H

#include <Vertex.h>
#include <vector>
#include <MeshImporter.h>
#include <QEMeshData.h>
#include <AnimationImporter.h>

class IQEMeshGenerator
{
public:
    virtual ~IQEMeshGenerator() = default;
    virtual QEMesh GenerateQEMesh() = 0;
};

// ConcreteStrategy Cube
class CubeGenerator : public IQEMeshGenerator
{
private:
    float size = 1.0f;
public:
    explicit CubeGenerator(float size) : size(size) {}
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        float h = size * 0.5f;

        // Definimos 8 posiciones
        glm::vec3 positions[8] = {
            {-h, -h, -h}, // 0
            { h, -h, -h}, // 1
            { h,  h, -h}, // 2
            {-h,  h, -h}, // 3
            {-h, -h,  h}, // 4
            { h, -h,  h}, // 5
            { h,  h,  h}, // 6
            {-h,  h,  h}  // 7
        };

        // Normales para cada cara
        glm::vec3 normals[6] = {
            { 0,  0, -1}, // atrás
            { 0,  0,  1}, // adelante
            {-1,  0,  0}, // izquierda
            { 1,  0,  0}, // derecha
            { 0,  1,  0}, // arriba
            { 0, -1,  0}, // abajo
        };

        // UVs estándar para un quad
        glm::vec2 uvs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f},
        };

        // Para obtener normales planas sin mezclar vértices,
        // duplicamos vértices por cara (24 en total).
        // Índices de las 6 caras (2 triángulos × 3 índices):
        unsigned int faceIndices[36] = {
            0,  1,  2,   2,  3,  0,   // atrás
            4,  5,  6,   6,  7,  4,   // adelante
            8,  9, 10,  10, 11,  8,   // izquierda
           12, 13, 14,  14, 15, 12,   // derecha
           16, 17, 18,  18, 19, 16,   // arriba
           20, 21, 22,  22, 23, 20    // abajo
        };

        // Construcción de vértices duplicados por cara
        for (int face = 0; face < 6; ++face)
        {
            int base = face * 4;
            // Elige las 4 esquinas de esa cara
            unsigned int idx[4];
            switch (face) {
            case 0: idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 3; break; // atrás
            case 1: idx[0] = 4; idx[1] = 5; idx[2] = 6; idx[3] = 7; break; // adelante
            case 2: idx[0] = 0; idx[1] = 4; idx[2] = 7; idx[3] = 3; break; // izquierda
            case 3: idx[0] = 1; idx[1] = 2; idx[2] = 6; idx[3] = 5; break; // derecha
            case 4: idx[0] = 3; idx[1] = 2; idx[2] = 6; idx[3] = 7; break; // arriba
            case 5: idx[0] = 0; idx[1] = 1; idx[2] = 5; idx[3] = 4; break; // abajo
            }
            // Para cada una de las 4 esquinas
            for (int corner = 0; corner < 4; ++corner)
            {
                meshData.Vertices.emplace_back(
                    positions[idx[corner]],  // posición
                    normals[face],           // normal plana
                    uvs[corner]              // coordenada UV
                );
            }
        }

        // Copiamos los índices relativos a los vértices recién añadidos
        meshData.Indices.reserve(36);
        for (int i = 0; i < 36; ++i)
        {
            meshData.Indices.push_back(faceIndices[i]);
        }

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-h, -h, -h), glm::vec3(h, h, h));

        return QEMesh("CubePrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Sphere
class SphereGenerator : public IQEMeshGenerator
{
private:
    float radius = 0.5f;
    uint32_t sectorCount = 36;
    uint32_t stackCount = 18;

public:
    explicit SphereGenerator(float r, uint32_t stacks, uint32_t sectors)
        : radius(r), stackCount(stacks), sectorCount(sectors) {
    }
    QEMesh GenerateQEMesh() override
    {

        QEMeshData meshData;
        auto& V = meshData.Vertices;
        auto& I = meshData.Indices;

        // Precompute step angles
        const float PI = glm::pi<float>();
        float const sectorStep = 2 * PI / sectorCount;
        float const stackStep = PI / stackCount;

        // Generate vertices
        for (uint32_t i = 0; i <= stackCount; ++i) {
            float stackAngle = PI / 2 - i * stackStep;        // from +pi/2 to -pi/2
            float xy = radius * cosf(stackAngle);             // r * cos(u)
            float z = radius * sinf(stackAngle);             // r * sin(u)

            for (uint32_t j = 0; j <= sectorCount; ++j) {
                float sectorAngle = j * sectorStep;           // from 0 to 2pi

                // Position (x, y, z)
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                glm::vec3 pos3(x, y, z);

                // Normalized normal
                glm::vec3 norm = glm::normalize(pos3);

                // UV coordinates (s,t)
                float s = (float)j / sectorCount;
                float t = (float)i / stackCount;

                // Tangent vector (pointing along +longitude)
                glm::vec3 tangent = glm::normalize(glm::vec3(
                    -radius * sinf(stackAngle) * sinf(sectorAngle),
                    radius * sinf(stackAngle) * cosf(sectorAngle),
                    0.0f
                ));

                Vertex vert;
                vert.Position = glm::vec4(pos3, 1.0f);
                vert.Normal = glm::vec4(norm, 0.0f);
                vert.UV = glm::vec2(s, t);
                vert.Tangent = glm::vec4(tangent, 0.0f);

                V.push_back(vert);
            }
        }

        // Generate indices (two triangles per quad)
        for (uint32_t i = 0; i < stackCount; ++i) {
            uint32_t k1 = i * (sectorCount + 1);     // beginning of current stack
            uint32_t k2 = k1 + sectorCount + 1;      // beginning of next stack

            for (uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                if (i != 0) {
                    // k1, k2, k1+1
                    I.push_back(k1);
                    I.push_back(k2);
                    I.push_back(k1 + 1);
                }
                if (i != (stackCount - 1)) {
                    // k1+1, k2, k2+1
                    I.push_back(k1 + 1);
                    I.push_back(k2);
                    I.push_back(k2 + 1);
                }
            }
        }

        // Compute AABB (axis-aligned bounding box)
        glm::vec3 minB(std::numeric_limits<float>::max());
        glm::vec3 maxB(-std::numeric_limits<float>::max());
        for (auto& vert : V) {
            glm::vec3 p = glm::vec3(vert.Position);
            minB = glm::min(minB, p);
            maxB = glm::max(maxB, p);
        }
        meshData.BoundingBox = { minB, maxB };

        return QEMesh("SpherePrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Point
class PointGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        meshData.Vertices.resize(1);
        Vertex vert;

        vert.Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        vert.Normal = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        vert.Tangent = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        meshData.Vertices[0] = vert;

        meshData.Indices.resize(1);
        meshData.Indices = { 0 };

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-0.1f), glm::vec3(0.1f));

        return QEMesh("PointPrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Triangle
class TriangleGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        meshData.Vertices.resize(3);
        Vertex vert;

        vert.Position = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 1.0f);
        meshData.Vertices[0] = vert;

        vert.Position = glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        meshData.Vertices[1] = vert;

        vert.Position = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 0.0f);
        meshData.Vertices[2] = vert;

        meshData.Indices.resize(3);
        meshData.Indices = { 0, 2, 1 };

        MeshImporter::RecreateNormals(meshData.Vertices, meshData.Indices);
        MeshImporter::RecreateTangents(meshData.Vertices, meshData.Indices);

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f));

        return QEMesh("TrianglePrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Quad
class QuadGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        meshData.Vertices.resize(4);
        Vertex vert;

        vert.Position = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 1.0f);
        meshData.Vertices[0] = vert;

        vert.Position = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        meshData.Vertices[1] = vert;

        vert.Position = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 0.0f);
        meshData.Vertices[2] = vert;

        vert.Position = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 1.0f);
        meshData.Vertices[3] = vert;

        meshData.Indices.resize(6);
        meshData.Indices = { 0, 2, 1, 0, 1, 3 };

        MeshImporter::RecreateNormals(meshData.Vertices, meshData.Indices);
        MeshImporter::RecreateTangents(meshData.Vertices, meshData.Indices);

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f));

        return QEMesh("QuadPrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Floor
class FloorGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        meshData.Vertices.resize(4);
        Vertex vert;

        vert.Position = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 1.0f);
        meshData.Vertices[0] = vert;

        vert.Position = glm::vec4(-1.0f, 0.0f, -1.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        meshData.Vertices[1] = vert;

        vert.Position = glm::vec4(-1.0f, 0.0f, 1.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 0.0f);
        meshData.Vertices[2] = vert;

        vert.Position = glm::vec4(1.0f, 0.0f, -1.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 1.0f);
        meshData.Vertices[3] = vert;

        meshData.Indices.resize(6);
        meshData.Indices = { 0, 1, 2, 0, 3, 1 };

        MeshImporter::RecreateNormals(meshData.Vertices, meshData.Indices);
        MeshImporter::RecreateTangents(meshData.Vertices, meshData.Indices);

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-1.0f, -0.001f, -1.0f), glm::vec3(1.0f, 0.001f, 1.0f));

        return QEMesh("FloorPrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Grid
class GridGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

        meshData.Vertices.resize(6);
        Vertex vert;

        vert.Position = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 1.0f);
        meshData.Vertices[0] = vert;

        vert.Position = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 1.0f);
        meshData.Vertices[1] = vert;

        vert.Position = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        meshData.Vertices[2] = vert;

        vert.Position = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 1.0f);
        meshData.Vertices[3] = vert;

        vert.Position = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(0.0f, 0.0f);
        meshData.Vertices[4] = vert;

        vert.Position = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
        vert.UV = glm::vec2(1.0f, 0.0f);
        meshData.Vertices[5] = vert;

        meshData.Indices.resize(6);
        meshData.Indices = { 0, 1, 2, 3, 4, 5 };

        MeshImporter::RecreateNormals(meshData.Vertices, meshData.Indices);
        MeshImporter::RecreateTangents(meshData.Vertices, meshData.Indices);

        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 1.0f));

        return QEMesh("GridPrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Capsule
class CapsuleGenerator : public IQEMeshGenerator
{
private:
    float capsuleRadius = 0.5f;
    float capsuleHeight = 1.0f;
    int capsuleSegments = 16;
    int capsuleRings = 8;

public:
    explicit CapsuleGenerator(int segments, int rings, float radius, float height)
        : capsuleSegments(segments), capsuleRings(rings), capsuleRadius(radius), capsuleHeight(height) {
    }
    QEMesh GenerateQEMesh() override
    {
        QEMeshData meshData;

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
                vertex.Position = glm::vec4(x, y, z, 1.0f);
                meshData.Vertices.push_back(vertex);
            }
        }

        int cylinderVerts = (this->capsuleRings + 1) * (this->capsuleSegments + 1);

        for (int i = 0; i < this->capsuleRings; ++i)
        {
            for (int j = 0; j < this->capsuleSegments; ++j)
            {
                int a = i * (this->capsuleSegments + 1) + j;
                int b = a + this->capsuleSegments + 1;

                meshData.Indices.push_back(a);
                meshData.Indices.push_back(b);
                meshData.Indices.push_back(a + 1);

                meshData.Indices.push_back(b);
                meshData.Indices.push_back(b + 1);
                meshData.Indices.push_back(a + 1);
            }
        }

        // ---- 2. HEMISFERIOS ----
        // Superior
        int baseIndex = (int)meshData.Vertices.size();
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
                vertex.Position = glm::vec4(glm::vec3(x, y + this->capsuleHeight / 2.0f, z), 1.0f);
                meshData.Vertices.push_back(vertex);
            }
        }

        int hemisphereRings = this->capsuleRings;
        for (int i = 0; i < hemisphereRings; ++i)
        {
            for (int j = 0; j < this->capsuleSegments; ++j)
            {
                int a = baseIndex + i * (this->capsuleSegments + 1) + j;
                int b = a + this->capsuleSegments + 1;

                meshData.Indices.push_back(a);
                meshData.Indices.push_back(b);
                meshData.Indices.push_back(a + 1);

                meshData.Indices.push_back(b);
                meshData.Indices.push_back(b + 1);
                meshData.Indices.push_back(a + 1);
            }
        }

        // Inferior (espejado)
        baseIndex = (int)meshData.Vertices.size();
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
                vertex.Position = glm::vec4(glm::vec3(x, y - this->capsuleHeight / 2.0f, z), 1.0f);
                meshData.Vertices.push_back(vertex);
            }
        }

        for (int i = 0; i < hemisphereRings; ++i)
        {
            for (int j = 0; j < this->capsuleSegments; ++j)
            {
                int a = baseIndex + i * (this->capsuleSegments + 1) + j;
                int b = a + this->capsuleSegments + 1;

                meshData.Indices.push_back(a);
                meshData.Indices.push_back(a + 1);
                meshData.Indices.push_back(b);

                meshData.Indices.push_back(b);
                meshData.Indices.push_back(a + 1);
                meshData.Indices.push_back(b + 1);
            }
        }

        MeshImporter::RecreateNormals(meshData.Vertices, meshData.Indices);
        MeshImporter::RecreateTangents(meshData.Vertices, meshData.Indices);

        glm::vec3 minValue = glm::vec3(-this->capsuleRadius, -this->capsuleHeight / 2.0f, -this->capsuleRadius);
        glm::vec3 maxValue = glm::vec3(this->capsuleRadius, this->capsuleHeight / 2.0f, this->capsuleRadius);
        meshData.BoundingBox = std::pair<glm::vec3, glm::vec3>(minValue, maxValue);

        return QEMesh("CapsulePrimitive", "QECore", { meshData });
    }
};

// ConcreteStrategy Mesh
class MeshGenerator : public IQEMeshGenerator
{
private:
    string dataPath;
public:
    explicit MeshGenerator(string data)
        : dataPath(data) {
    }
    QEMesh GenerateQEMesh() override
    {
        QEMesh mesh = MeshImporter::LoadMesh(dataPath);

        if (mesh.MeshData.empty())
        {
            return QEMesh("EmptyMesh", "QECore", {});
        }

        if (!mesh.BonesInfoMap.empty())
        {
            mesh.AnimationData = AnimationImporter::ImportAnimation(dataPath, mesh.BonesInfoMap, mesh.BonesInfoMap.size());
        }

        return mesh;
    }
};

#endif // !MESH_DATA_H

