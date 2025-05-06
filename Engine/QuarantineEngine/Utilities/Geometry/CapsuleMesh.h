#pragma once

#ifndef CAPSULE_MESH_H
#define CAPSULE_MESH_H

#include "PrimitiveMesh.h"
class CapsuleMesh : public PrimitiveMesh
{
private:
    float capsuleRadius = 0.5f;
    float capsuleHeight = 1.0f;
    int capsuleSegments = 16;
    int capsuleRings = 8;

private:
    void GenerateCapsuleMesh();

public:
    CapsuleMesh();

    float GetCapsuleHeight() { return this->capsuleHeight; }
    float GetCapsuleRadius() { return this->capsuleRadius; }
    int GetCapsuleSegments() { return this->capsuleSegments; }
    int GetCapsuleRings() { return this->capsuleRings; }

    void InitializeMesh() override;
    void SetCapsuleHeight(float height) { this->capsuleHeight = height; }
    void SetCapsuleRadius(float radius) { this->capsuleRadius = radius; }
    void SetCapsuleSegments(int segments) { this->capsuleSegments = segments; }
    void SetCapsuleRings(int rings) { this->capsuleRings = rings; }
};

#endif

