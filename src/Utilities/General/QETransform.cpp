#include "QETransform.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <cmath>
#include <limits>

namespace
{
    constexpr float QE_TRANSFORM_MIN_SCALE = 0.001f;
    constexpr float QE_TRANSFORM_MAX_SCALE = 100000.0f;

    bool IsFiniteVec3(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    bool IsFiniteQuat(const glm::quat& q)
    {
        return std::isfinite(q.x) && std::isfinite(q.y) &&
            std::isfinite(q.z) && std::isfinite(q.w);
    }

    float SanitizeScaleComponent(float v, float fallback)
    {
        if (!std::isfinite(v))
            return fallback;

        // Si no quieres permitir escalado negativo en el engine:
        v = std::abs(v);

        if (v < QE_TRANSFORM_MIN_SCALE)
            v = QE_TRANSFORM_MIN_SCALE;

        if (v > QE_TRANSFORM_MAX_SCALE)
            v = QE_TRANSFORM_MAX_SCALE;

        return v;
    }

    glm::vec3 SanitizeScale(const glm::vec3& s, const glm::vec3& fallback)
    {
        return glm::vec3(
            SanitizeScaleComponent(s.x, fallback.x),
            SanitizeScaleComponent(s.y, fallback.y),
            SanitizeScaleComponent(s.z, fallback.z));
    }

    glm::quat SafeNormalizeQuat(const glm::quat& q, const glm::quat& fallback)
    {
        if (!IsFiniteQuat(q))
            return fallback;

        float len = glm::length(q);
        if (!std::isfinite(len) || len < 1e-8f)
            return fallback;

        return glm::normalize(q);
    }
}

QETransform::QETransform()
{
    localPosition = glm::vec3(0.0f);
    localRotation = glm::quat(1,0,0,0);
    localScale = glm::vec3(1.0f);
}

void QETransform::EnsureLocal()
{
    if (!localDirty) return;

    localMatrix = glm::translate(glm::mat4(1.0f), localPosition)
        * glm::toMat4(localRotation)
        * glm::scale(glm::mat4(1.0f), localScale);
    localDirty = false;
}

void QETransform::EnsureWorld()
{
    if (!worldDirty) return;

    EnsureLocal();
    if (auto p = parent.lock())
    {
        worldMatrix = p->GetWorldMatrix() * localMatrix;
    }
    else
    {
        worldMatrix = localMatrix;
    }
    worldDirty = false;
}

void QETransform::MarkWorldDirty()
{
    if (worldDirty) return;
    worldDirty = true;
    worldVersion++;
    for (auto& c : children) c->MarkWorldDirty();
}

// -------- Setters locales
void QETransform::SetLocalPosition(const glm::vec3& p)
{
    localPosition = p; localDirty = true;
    MarkWorldDirty();
}

void QETransform::SetLocalRotation(const glm::quat& q)
{
    localRotation = glm::normalize(q); localDirty = true;
    MarkWorldDirty();
}

void QETransform::SetLocalEulerDegrees(const glm::vec3& deg)
{
    SetLocalRotation(glm::quat(glm::radians(deg)));
}

void QETransform::SetLocalScale(const glm::vec3& s)
{
    localScale = s; localDirty = true;
    MarkWorldDirty();
}

// -------- Movimientos
void QETransform::TranslateLocal(const glm::vec3& d)
{
    localPosition += (glm::toMat3(localRotation) * d);
    localDirty = true;
    MarkWorldDirty();
}

void QETransform::TranslateWorld(const glm::vec3& dWorld)
{
    constexpr float kEps = 1e-8f;
    if (auto p = parent.lock())
    {
        glm::quat pw = p->GetWorldRotation();
        glm::vec3 ps = p->GetWorldScale();

        glm::vec3 unscaled = dWorld / glm::max(ps, glm::vec3(kEps));
        glm::vec3 dLocalParent = glm::inverse(pw) * unscaled;
        localPosition += dLocalParent;
    }
    else
    {
        localPosition += dWorld;
    }
    localDirty = true;
    MarkWorldDirty();
}

void QETransform::RotateLocal(const glm::quat& dq)
{
    localRotation = glm::normalize(dq * localRotation);
    localDirty = true;
    MarkWorldDirty();
}

void QETransform::RotateWorld(const glm::quat& dq)
{
    if (auto p = parent.lock())
    {
        glm::quat pw = p->GetWorldRotation();
        glm::quat pl = glm::inverse(pw) * dq * pw;
        RotateLocal(pl);
    }
    else
    {
        RotateLocal(dq);
    }
}

void QETransform::SetFromMatrix(const glm::mat4& m)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::vec3 translation, scale;
    glm::quat rotation;

    const glm::vec3 oldPosition = localPosition;
    const glm::quat oldRotation = localRotation;
    const glm::vec3 oldScale = localScale;

    bool ok = glm::decompose(m, scale, rotation, translation, skew, perspective);
    if (!ok)
        return;

    if (!IsFiniteVec3(translation))
        translation = oldPosition;

    rotation = SafeNormalizeQuat(rotation, oldRotation);
    scale = SanitizeScale(scale, oldScale);

    localPosition = translation;
    localRotation = rotation;
    localScale = scale;

    localDirty = true;
    MarkWorldDirty();
}

// -------- Parenting
void QETransform::AddChild(const std::shared_ptr<QETransform>& child)
{
    if (!child) return;
    child->SetParent(GetPtr(), /*keepWorld=*/true);
}

void QETransform::SetParent(std::shared_ptr<QETransform> newParent, bool keepWorld)
{
    auto self = _self.lock();
    if (!self)
    {
        std::cerr << "[QETransform] SetParent called but self not set.\n";
        return;
    }

    glm::mat4 currentWorld = GetWorldMatrix();

    if (auto p = parent.lock())
    {
        auto& vec = p->children;
        vec.erase(std::remove(vec.begin(), vec.end(), self), vec.end());
    }
    parent.reset();

    if (newParent)
    {
        parent = newParent;
        newParent->children.push_back(self);
    }

    if (keepWorld)
    {
        glm::mat4 parentWorld = newParent ? newParent->GetWorldMatrix() : glm::mat4(1.0f);
        glm::mat4 newLocal = glm::inverse(parentWorld) * currentWorld;

        glm::vec3 skew;
        glm::vec4 persp;
        glm::vec3 t, s;
        glm::quat r;

        bool ok = glm::decompose(newLocal, s, r, t, skew, persp);
        if (ok)
        {
            if (!IsFiniteVec3(t))
                t = localPosition;

            r = SafeNormalizeQuat(r, localRotation);
            s = SanitizeScale(s, localScale);

            localPosition = t;
            localRotation = r;
            localScale = s;
            localDirty = true;
        }
    }

    MarkWorldDirty();
}

// -------- Getters matrices y derivados
const glm::mat4& QETransform::GetLocalMatrix() { EnsureLocal();  return localMatrix; }
const glm::mat4& QETransform::GetWorldMatrix() { EnsureWorld();  return worldMatrix; }

glm::vec3 QETransform::GetWorldPosition()
{
    EnsureWorld();
    return glm::vec3(worldMatrix[3]);
}

glm::quat QETransform::GetWorldRotation()
{
    EnsureWorld();
    glm::mat3 m(worldMatrix);

    glm::vec3 x = glm::normalize(glm::vec3(m[0]));
    glm::vec3 y = glm::normalize(glm::vec3(m[1]));
    glm::vec3 z = glm::normalize(glm::vec3(m[2]));
    glm::mat3 R(x, y, z);
    return glm::normalize(glm::quat_cast(R));
}

glm::vec3 QETransform::GetWorldScale()
{
    EnsureWorld();

    glm::mat3 m(worldMatrix);
    return glm::vec3(glm::length(m[0]), glm::length(m[1]), glm::length(m[2]));
}

glm::vec3 QETransform::Forward()
{
    return glm::normalize(GetWorldRotation() * glm::vec3(0, 0, -1));
}

glm::vec3 QETransform::Right() { return glm::normalize(GetWorldRotation() * glm::vec3(1, 0, 0)); }

glm::vec3 QETransform::Up() { return glm::normalize(GetWorldRotation() * glm::vec3(0, 1, 0)); }

void QETransform::Debug_PrintModel() const
{
    auto& M = const_cast<QETransform*>(this)->GetWorldMatrix();
    for (int i = 0; i < 4; i++) {
        printf("% .4f % .4f % .4f % .4f\n", M[i][0], M[i][1], M[i][2], M[i][3]);
    }
    puts("---------------");
}

void QETransform::QEUpdate()
{
    if (!localDirty && !worldDirty)
        return;

    EnsureWorld();
}
