#include "DirectionalLight.h"
#include <SynchronizationModule.h>
#include <QESessionManager.h>
#include "QECamera.h"

QEDirectionalLight::QEDirectionalLight() : QELight()
{
    this->lightType = LightType::DIRECTIONAL_LIGHT;
    this->radius = FLT_MAX;
    this->cascadeSplitLambda = 0.9f;

    EnsureRuntimeState();
}

void QEDirectionalLight::Setup(std::shared_ptr<VkRenderPass> renderPass)
{
    EnsureRuntimeState();
    this->shadowMappingResourcesPtr = std::make_shared<CSMResources>(renderPass);
}

void QEDirectionalLight::UpdateUniform()
{
    EnsureRuntimeState();

    if (!this->transform)
        return;

    if (!this->shadowMappingResourcesPtr)
        return;

    auto activeCamera = QESessionManager::getInstance()->ActiveCamera();
    if (!activeCamera)
        return;

    QELight::UpdateUniform();

    this->uniform->position = this->transform->GetWorldPosition();
    this->uniform->direction = this->transform->Forward();

    this->UpdateCascades();
    this->shadowMappingResourcesPtr->UpdateOffscreenUBOShadowMap();
}

void QEDirectionalLight::UpdateCascades()
{
    EnsureRuntimeState();

    auto activeCamera = QESessionManager::getInstance()->ActiveCamera();
    if (!activeCamera)
        return;

    if (!this->transform)
        return;

    if (!this->shadowMappingResourcesPtr || !this->shadowMappingResourcesPtr->CascadeResourcesPtr)
        return;

    float nearClip = activeCamera->GetNear();
    float farClip = activeCamera->GetFar();
    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;
    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    float cascadeSplitPtr[SHADOW_MAP_CASCADE_COUNT];

    // GPU Gems split scheme (resultado: splitDist 0..1)
    for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
        float log = minZ * std::pow(ratio, p);
        float uni = minZ + range * p;
        float d = this->cascadeSplitLambda * (log - uni) + uni;
        cascadeSplitPtr[i] = (d - nearClip) / clipRange;
    }

    float lastSplitDist = 0.0f;

    // Matriz inversa de cámara (NDC->World)
    glm::mat4 invCam = glm::inverse(activeCamera->CameraData->ViewProjection);

    const float shadowMapRes = this->shadowMappingResourcesPtr
        ? static_cast<float>(this->shadowMappingResourcesPtr->TextureSize)
        : 2048.0f;

    for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
    {
        float splitDist = cascadeSplitPtr[i];

        // NDC corners en Vulkan: z ∈ [0..1]
        glm::vec3 frustumCorners[8] = {
            {-1.0f,  1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f},
            {-1.0f, -1.0f, 0.0f},
            {-1.0f,  1.0f, 1.0f},
            { 1.0f,  1.0f, 1.0f},
            { 1.0f, -1.0f, 1.0f},
            {-1.0f, -1.0f, 1.0f}
        };

        // Project frustum corners into world space
        for (uint32_t j = 0; j < 8; j++)
        {
            glm::vec4 w = invCam * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = glm::vec3(w) / w.w;
        }

        // Slice el frustum usando lastSplitDist/splitDist (0..1)
        for (uint32_t j = 0; j < 4; j++)
        {
            const glm::vec3 cornerNear = frustumCorners[j];
            const glm::vec3 cornerFar = frustumCorners[j + 4];
            const glm::vec3 dist = cornerFar - cornerNear;

            frustumCorners[j] = cornerNear + dist * lastSplitDist;
            frustumCorners[j + 4] = cornerNear + dist * splitDist;
        }

        // Centro del slice
        glm::vec3 frustumCenter(0.0f);
        for (uint32_t j = 0; j < 8; j++) frustumCenter += frustumCorners[j];
        frustumCenter /= 8.0f;

        // Usamos una esfera envolvente para estabilizar las cascadas frente a la rotacion
        // de la camara. Un AABB ajustado en light-space reduce cobertura, pero provoca
        // cambios visibles entre cascadas cuando rota el frustum.
        float radius = 0.0f;
        for (uint32_t j = 0; j < 8; ++j)
        {
            radius = std::max(radius, glm::length(frustumCorners[j] - frustumCenter));
        }
        radius = std::max(radius, 1.0f);
        radius = std::ceil(radius * 16.0f) / 16.0f;

        // Dirección de luz normalizada
        glm::vec3 lightDir = glm::normalize(this->transform->Forward());

        // Up robusto (evita degeneración si lightDir ~ (0,±1,0))
        glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);

        // Primero construye una view provisional (eye no crítico aún)
        glm::vec3 eye = frustumCenter - lightDir * 100.0f;
        glm::mat4 lightView = glm::lookAtRH(eye, frustumCenter, up);

        // Bounds en light-space. Z sigue saliendo de los corners reales para no recortar casters,
        // pero X/Y se estabilizan con la esfera envolvente.
        glm::vec3 minLS(std::numeric_limits<float>::max());
        glm::vec3 maxLS(-std::numeric_limits<float>::max());

        for (uint32_t j = 0; j < 8; j++)
        {
            glm::vec4 v = lightView * glm::vec4(frustumCorners[j], 1.0f);
            minLS = glm::min(minLS, glm::vec3(v));
            maxLS = glm::max(maxLS, glm::vec3(v));
        }

        glm::vec3 centerLS = glm::vec3(lightView * glm::vec4(frustumCenter, 1.0f));

        minLS.x = centerLS.x - radius;
        maxLS.x = centerLS.x + radius;
        minLS.y = centerLS.y - radius;
        maxLS.y = centerLS.y + radius;

        // Margen para casters/PCF (ajusta segun escena)
        const float zMargin = 50.0f;
        minLS.z -= zMargin;
        maxLS.z += zMargin;

        // Snap del centro del volumen a la rejilla de texels para eliminar swimming.
        float width = (maxLS.x - minLS.x);
        float height = (maxLS.y - minLS.y);

        // Evita división por cero
        width = std::max(width, 1e-4f);
        height = std::max(height, 1e-4f);

        float unitsPerTexelX = width / shadowMapRes;
        float unitsPerTexelY = height / shadowMapRes;

        centerLS.x = std::floor(centerLS.x / unitsPerTexelX) * unitsPerTexelX;
        centerLS.y = std::floor(centerLS.y / unitsPerTexelY) * unitsPerTexelY;

        minLS.x = centerLS.x - width * 0.5f;
        maxLS.x = centerLS.x + width * 0.5f;
        minLS.y = centerLS.y - height * 0.5f;
        maxLS.y = centerLS.y + height * 0.5f;

        // En lookAtRH, los puntos delante suelen tener z negativo.
        // Para orthoRH_ZO: near/far son positivos a lo largo del -Z de la cámara (light view).
        float nearZ = -maxLS.z;
        float farZ = -minLS.z;

        // Si por convención te quedara invertido, asegúrate:
        if (nearZ < 0.0f) nearZ = 0.0f;
        if (farZ < nearZ + 1e-3f) farZ = nearZ + 1.0f;

        glm::mat4 lightOrtho = glm::orthoRH_ZO(minLS.x, maxLS.x, minLS.y, maxLS.y, nearZ, farZ);

        // Guarda split end en unidades reales (positivo)
        float splitEnd = nearClip + splitDist * clipRange;
        this->shadowMappingResourcesPtr->CascadeResourcesPtr->at(i).splitDepth = splitEnd;

        // Matriz final
        this->shadowMappingResourcesPtr->CascadeResourcesPtr->at(i).viewProjMatrix = lightOrtho * lightView;

        lastSplitDist = splitDist;
    }
}

void QEDirectionalLight::EnsureRuntimeState()
{
    if (ResolveTransformFromOwner())
    {
        return;
    }

    if (this->transform == nullptr)
    {
        this->transform = std::make_shared<QETransform>();
        this->transform->SetLocalPosition(glm::vec3(0.0f, 10.0f, 0.0f));
        this->transform->SetLocalEulerDegrees(glm::vec3(90.0f, 0.0f, 0.0f));
    }
}

void QEDirectionalLight::CleanShadowMapResources()
{
    if (!this->shadowMappingResourcesPtr)
        return;

    this->shadowMappingResourcesPtr->Cleanup();
    this->shadowMappingResourcesPtr.reset();
    this->shadowMappingResourcesPtr = nullptr;
}
