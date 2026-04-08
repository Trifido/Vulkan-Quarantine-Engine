#pragma once

#ifndef IQE_MESH_GENERATOR_H
#define IQE_MESH_GENERATOR_H

#include <Vertex.h>
#include <vector>
#include <MeshImporter.h>
#include <QEMeshData.h>
#include <AnimationImporter.h>
#include <unordered_map>
#include <string>

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
    CubeGenerator() = default;
    CubeGenerator(float size) : size(size) {}
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Sphere
class SphereGenerator : public IQEMeshGenerator
{
private:
    float radius = 0.5f;
    uint32_t sectorCount = 36;
    uint32_t stackCount = 18;

public:
    SphereGenerator() = default;
    SphereGenerator(float r, uint32_t stacks, uint32_t sectors)
        : radius(r), stackCount(stacks), sectorCount(sectors) {
    }
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Point
class PointGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Triangle
class TriangleGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Quad
class QuadGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Floor
class FloorGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Grid
class GridGenerator : public IQEMeshGenerator
{
public:
    QEMesh GenerateQEMesh() override;
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
    explicit CapsuleGenerator() {}
    explicit CapsuleGenerator(int segments, int rings, float radius, float height)
        : capsuleSegments(segments), capsuleRings(rings), capsuleRadius(radius), capsuleHeight(height) {
    }
    QEMesh GenerateQEMesh();
};

// ConcreteStrategy Cylinder
class CylinderGenerator : public IQEMeshGenerator
{
private:
    float radius = 0.5f;
    float height = 1.0f;
    uint32_t radialSegments = 24;
    uint32_t heightSegments = 1;

public:
    CylinderGenerator() = default;
    CylinderGenerator(float r, float h, uint32_t radialSegs = 24, uint32_t heightSegs = 1)
        : radius(r), height(h), radialSegments(radialSegs), heightSegments(heightSegs) {
    }

    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Pyramid4
class PyramidGenerator : public IQEMeshGenerator
{
private:
    float baseSize = 1.0f;
    float height = 1.0f;

public:
    PyramidGenerator() = default;
    PyramidGenerator(float base, float h)
        : baseSize(base), height(h) {
    }

    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Torus
class TorusGenerator : public IQEMeshGenerator
{
private:
    float majorRadius = 0.75f;      
    float minorRadius = 0.25f;
    uint32_t majorSegments = 32;
    uint32_t minorSegments = 16;

public:
    TorusGenerator() = default;
    TorusGenerator(float majorR, float minorR, uint32_t majorSegs = 32, uint32_t minorSegs = 16)
        : majorRadius(majorR), minorRadius(minorR), majorSegments(majorSegs), minorSegments(minorSegs) {
    }

    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Cone
class ConeGenerator : public IQEMeshGenerator
{
private:
    float radius = 0.5f;
    float height = 1.0f;
    uint32_t radialSegments = 24;
    uint32_t heightSegments = 1;

public:
    ConeGenerator() = default;
    ConeGenerator(float r, float h, uint32_t radialSegs = 24, uint32_t heightSegs = 1)
        : radius(r), height(h), radialSegments(radialSegs), heightSegments(heightSegs) {
    }

    QEMesh GenerateQEMesh() override;
};

// ConcreteStrategy Mesh
class QEMeshGenerator : public IQEMeshGenerator
{
private:
    std::string dataPath;
public:
    QEMeshGenerator() = default;
    QEMeshGenerator(std::string data)
        : dataPath(data) {
    }
    QEMesh GenerateQEMesh() override;
};

#endif // !MESH_DATA_H
