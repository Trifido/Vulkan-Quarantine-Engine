#include "FrustumComponent.h"

FrustumComponent::FrustumComponent()
{
    this->initCorners[0] = glm::vec4(-1, -1, -1, 1);
    this->initCorners[1] = glm::vec4(1, -1, -1, 1);
    this->initCorners[2] = glm::vec4(1, 1, -1, 1);
    this->initCorners[3] = glm::vec4(-1, 1, -1, 1);
    this->initCorners[4] = glm::vec4(-1, -1, 1, 1);
    this->initCorners[5] = glm::vec4(1, -1, 1, 1);
    this->initCorners[6] = glm::vec4(1, 1, 1, 1);
    this->initCorners[7] = glm::vec4(-1, 1, 1, 1);
}

void FrustumComponent::ActivateComputeCulling(bool value)
{
    this->isComputeCullingEnable = value;
}

bool FrustumComponent::IsComputeCullingActive()
{
    return this->isComputeCullingEnable;
}

void FrustumComponent::RecreateFrustumCorners(glm::mat4 viewProjection)
{
    const glm::mat4 inverseViewProj = glm::inverse(viewProjection);

    for (int i = 0; i < 8; i++)
    {
        const glm::vec4 q = inverseViewProj * initCorners[i];
        corners[i] = q / q.w;
    }
}

void FrustumComponent::RecreateFrustum(glm::mat4 viewProjection)
{
    glm::mat4 viewprojectionTranspose = glm::transpose(viewProjection);
    this->frustumPlanes[0] = viewprojectionTranspose[3] + viewprojectionTranspose[0];
    this->frustumPlanes[1] = viewprojectionTranspose[3] - viewprojectionTranspose[0];
    this->frustumPlanes[2] = viewprojectionTranspose[3] + viewprojectionTranspose[1];
    this->frustumPlanes[3] = viewprojectionTranspose[3] - viewprojectionTranspose[1];
    this->frustumPlanes[4] = viewprojectionTranspose[3] + viewprojectionTranspose[2];
    this->frustumPlanes[5] = viewprojectionTranspose[3] - viewprojectionTranspose[2];

    this->RecreateFrustumCorners(viewProjection);
}

bool FrustumComponent::isAABBInside(AABBObject& box)
{
    auto model = box.GetTransform()->GetWorldMatrix();
    glm::vec4 min = model * glm::vec4(box.min, 1.0f);
    glm::vec4 max = model * glm::vec4(box.max, 1.0f);

    for (int i = 0; i < 6; i++)
    {
        int r = 0;

        r += (glm::dot(this->frustumPlanes[i], glm::vec4(min.x, min.y, min.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(max.x, min.y, min.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(min.x, max.y, min.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(max.x, max.y, min.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(min.x, min.y, max.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(max.x, min.y, max.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(min.x, max.y, max.z, 1.0f)) < 0) ? 1 : 0;
        r += (glm::dot(this->frustumPlanes[i], glm::vec4(max.x, max.y, max.z, 1.0f)) < 0) ? 1 : 0;

        if (r == 8) return false;
    }

    int r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].x > max.x)) ? 1 : 0;
    if (r == 8) return false;

    r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].x < min.x)) ? 1 : 0;
    if (r == 8) return false;

    r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].y > max.y)) ? 1 : 0;
    if (r == 8) return false;

    r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].y < min.y)) ? 1 : 0;
    if (r == 8) return false;

    r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].z > max.z)) ? 1 : 0;
    if (r == 8) return false;

    r = 0;
    for (int i = 0; i < 8; i++)
        r += ((this->corners[i].z < min.z)) ? 1 : 0;
    if (r == 8) return false;

    return true;
}
