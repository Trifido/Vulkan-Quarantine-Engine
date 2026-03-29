# Sistema de Iluminación

> **Idioma:** [English](../en/Lighting-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

El sistema de iluminación reside en `src/Utilities/Light/` y soporta cuatro tipos de luz más un pipeline completo de sombras con mapas en cascada y omnidireccionales.

---

## Tipos de Luz

| Clase | Archivo | Tipo | Sombra |
|---|---|---|---|
| `DirectionalLight` | `DirectionalLight.h` | Rayos paralelos (tipo sol, distante) | Array de profundidad CSM |
| `SunLight` | `SunLight.h` | Direccional + integración atmosférica | Array de profundidad CSM |
| `PointLight` | `PointLight.h` | Omnidireccional, cae con la distancia | Sombra cubemap |
| `SpotLight` | `SpotLight.h` | En forma de cono, ángulo configurable | (planificado) |

Todas las luces son derivadas de `QEGameComponent` y se registran en `LightManager::Instance()` al crearlas.

### DirectionalLight

```cpp
auto* sol = obj->AddComponent<DirectionalLight>();
sol->SetDirection(glm::vec3(-0.5f, -1.0f, -0.3f));
sol->SetColor(glm::vec3(1.0f, 0.95f, 0.8f));
sol->SetIntensity(3.0f);
```

### PointLight

```cpp
auto* lampara = obj->AddComponent<PointLight>();
lampara->SetColor(glm::vec3(1.0f, 0.6f, 0.2f));
lampara->SetIntensity(5.0f);
lampara->SetRange(10.0f);
```

### SpotLight

```cpp
auto* foco = obj->AddComponent<SpotLight>();
foco->SetDirection(glm::vec3(0.0f, -1.0f, 0.0f));
foco->SetInnerConeAngle(15.0f);  // grados
foco->SetOuterConeAngle(30.0f);
foco->SetIntensity(8.0f);
```

---

## LightManager

`LightManager` (singleton) agrega todas las luces activas y sube sus datos a los uniform buffers de la GPU en cada frame.

```cpp
LightManager* lm = LightManager::Instance();
const auto& dirLights   = lm->GetDirectionalLights();
const auto& pointLights = lm->GetPointLights();
```

Las luces se serializan a un `LightDto` y se empaquetan en un SSBO / UBO para el fragment shader.

---

## Mapeo de Sombras

### Cascaded Shadow Maps (CSM)

CSM se usa para `DirectionalLight` y `SunLight` para proporcionar sombras de alta calidad a grandes distancias de visión.

**Clases principales:**

| Clase | Rol |
|---|---|
| `ShadowPipelineManager` | Posee el pipeline Vulkan para las pasadas de profundidad de sombra |
| `CSMResources` | Array de imágenes de profundidad, framebuffers por cascada |
| `CSMDescriptorsManager` | Descriptor sets que vinculan el array de texturas de sombra |

**Algoritmo:**

1. Dividir el frustum de visión en N cascadas (distribución logarítmica).
2. Para cada cascada, calcular una matriz de proyección ortográfica que encaje el fragmento del frustum.
3. Renderizar la profundidad de la escena en una capa del array de profundidad para esa cascada.
4. En la pasada principal, muestrear la capa de cascada correcta basándose en la profundidad del fragmento.

**Shader:** `resources/shaders/Shadow/csm.vert` / `csm.frag`  
**Helper GLSL:** `resources/shaders/Includes/QEShadows.glsl`

```glsl
// En el fragment shader (QEShadows.glsl)
float shadow = SampleCSM(fragPosLightSpace, cascadeIndex, shadowMap);
```

### Sombras Omnidireccionales (Luz Puntual)

Cada `PointLight` renderiza un cubemap de profundidad de su entorno.

**Clases principales:**

| Clase | Rol |
|---|---|
| `OmniShadowResources` | Imagen de cubemap de profundidad, 6 framebuffers de caras |
| `PointShadowDescriptorsManager` | Descriptor sets que vinculan el sampler de cubemap |

**Shader:** `resources/shaders/Shadow/omni_shadow.vert` / `omni_shadow.frag`

```glsl
// Muestrear sombra puntual en el fragment shader
float shadow = SampleOmniShadow(fragToLight, farPlane, shadowCubemap);
```

---

## Modelo de Iluminación PBR

El fragment shader por defecto (`default.frag`) implementa un **BRDF Cook-Torrance**:

```
L_out = ∫ (k_d * f_lambert + k_s * f_specular) * L_in * cos(θ) dω
```

Donde:
- `f_lambert` = `albedo / π`
- `f_specular` = `D * F * G / (4 * NdotV * NdotL)` (distribución GGX, Fresnel Schlick, geometría Smith)

Los helpers de shader están en `resources/shaders/Includes/PBR/`.

---

## Atmósfera y Cielo

`SunLight` se integra con `AtmosphereSystem` para:

- Dirigir la dirección del sol para la generación de la sky-view LUT.
- Aplicar perspectiva aérea a la geometría lejana de la cámara.

Consulta [Pipeline de Renderizado — Atmosphere Pass](Pipeline-Renderizado.md#atmosphere-pass) para más detalles.

---

## Ver también

- [Pipeline de Renderizado](Pipeline-Renderizado.md)
- [Sistema de Shaders](Sistema-Shaders.md)
- [Sistema de Materiales](Sistema-Materiales.md)
