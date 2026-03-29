# Lighting System

> **Language:** English · [Español](../es/Sistema-Iluminacion.md)  
> ← [Home](Home.md)

---

## Overview

The lighting system lives in `src/Utilities/Light/` and supports four light types plus a full shadow pipeline with cascaded and omnidirectional shadow maps.

---

## Light Types

| Class | File | Type | Shadow |
|---|---|---|---|
| `DirectionalLight` | `DirectionalLight.h` | Parallel rays (sun-like, distant) | CSM depth array |
| `SunLight` | `SunLight.h` | Directional + atmosphere integration | CSM depth array |
| `PointLight` | `PointLight.h` | Omnidirectional, falls off with distance | Cubemap shadow |
| `SpotLight` | `SpotLight.h` | Cone-shaped, configurable angle | (planned) |

All lights are `QEGameComponent`-derived and are registered with `LightManager::Instance()` on creation.

### DirectionalLight

```cpp
auto* sun = obj->AddComponent<DirectionalLight>();
sun->SetDirection(glm::vec3(-0.5f, -1.0f, -0.3f));
sun->SetColor(glm::vec3(1.0f, 0.95f, 0.8f));
sun->SetIntensity(3.0f);
```

### PointLight

```cpp
auto* lamp = obj->AddComponent<PointLight>();
lamp->SetColor(glm::vec3(1.0f, 0.6f, 0.2f));
lamp->SetIntensity(5.0f);
lamp->SetRange(10.0f);
```

### SpotLight

```cpp
auto* spot = obj->AddComponent<SpotLight>();
spot->SetDirection(glm::vec3(0.0f, -1.0f, 0.0f));
spot->SetInnerConeAngle(15.0f);  // degrees
spot->SetOuterConeAngle(30.0f);
spot->SetIntensity(8.0f);
```

---

## LightManager

`LightManager` (singleton) aggregates all active lights and uploads their data to GPU uniform buffers each frame.

```cpp
LightManager* lm = LightManager::Instance();
const auto& dirLights   = lm->GetDirectionalLights();
const auto& pointLights = lm->GetPointLights();
```

The lights are serialised to a `LightDto` and packed into an SSBO / UBO for the fragment shader.

---

## Shadow Mapping

### Cascaded Shadow Maps (CSM)

CSM is used for `DirectionalLight` and `SunLight` to provide high-quality shadows over large view distances.

**Key classes:**

| Class | Role |
|---|---|
| `ShadowPipelineManager` | Owns the Vulkan pipeline for shadow depth passes |
| `CSMResources` | Depth image array, per-cascade framebuffers |
| `CSMDescriptorsManager` | Descriptor sets that bind the shadow texture array |

**Algorithm:**

1. Split the view frustum into N cascades (logarithmic distribution).
2. For each cascade, compute an orthographic projection matrix fitting the frustum slice.
3. Render the scene depth-only into a depth array layer for that cascade.
4. In the main pass, sample the correct cascade layer based on fragment depth.

**Shader:** `resources/shaders/Shadow/csm.vert` / `csm.frag`  
**GLSL helper:** `resources/shaders/Includes/QEShadows.glsl`

```glsl
// In the fragment shader (QEShadows.glsl)
float shadow = SampleCSM(fragPosLightSpace, cascadeIndex, shadowMap);
```

### Omnidirectional (Point-Light) Shadows

Each `PointLight` renders a depth cubemap of its surrounding environment.

**Key classes:**

| Class | Role |
|---|---|
| `OmniShadowResources` | Depth cubemap image, 6 face framebuffers |
| `PointShadowDescriptorsManager` | Descriptor sets binding the cubemap sampler |

**Shader:** `resources/shaders/Shadow/omni_shadow.vert` / `omni_shadow.frag`

```glsl
// Sample point shadow in fragment shader
float shadow = SampleOmniShadow(fragToLight, farPlane, shadowCubemap);
```

---

## PBR Lighting Model

The default fragment shader (`default.frag`) implements a **Cook-Torrance BRDF**:

```
L_out = ∫ (k_d * f_lambert + k_s * f_specular) * L_in * cos(θ) dω
```

Where:
- `f_lambert` = `albedo / π`
- `f_specular` = `D * F * G / (4 * NdotV * NdotL)` (GGX distribution, Schlick Fresnel, Smith geometry)

Shader helpers are in `resources/shaders/Includes/PBR/`.

---

## Atmosphere & Sky

`SunLight` integrates with `AtmosphereSystem` to:

- Drive the sun direction for sky-view LUT generation.
- Apply aerial perspective to geometry far from the camera.

See [Rendering Pipeline — Atmosphere Pass](Rendering-Pipeline.md#atmosphere-pass) for details.

---

## See Also

- [Rendering Pipeline](Rendering-Pipeline.md)
- [Shader System](Shader-System.md)
- [Material System](Material-System.md)
