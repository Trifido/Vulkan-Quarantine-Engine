# Material System

> **Language:** English · [Español](../es/Sistema-Materiales.md)  
> ← [Home](Home.md)

---

## Overview

The material system (`src/Utilities/Material/`) manages PBR surface properties, texture slots, and GPU resource allocation for all rendered geometry.

---

## Material

`Material` is a `QEGameComponent` that stores PBR parameters and a set of texture slots.

### PBR Properties

| Property | Type | Default | Description |
|---|---|---|---|
| `albedo` | `glm::vec3` | `(1,1,1)` | Base colour (linear) |
| `metallic` | `float` | `0.0` | 0 = dielectric, 1 = metallic |
| `roughness` | `float` | `0.5` | Microsurface roughness |
| `ambientOcclusion` | `float` | `1.0` | Ambient occlusion multiplier |
| `emissive` | `glm::vec3` | `(0,0,0)` | Self-emission colour |
| `emissiveIntensity` | `float` | `0.0` | Emission brightness multiplier |
| `opacity` | `float` | `1.0` | Alpha (1 = opaque) |

### Texture Slots

| Slot | Enum | Typical Map |
|---|---|---|
| Albedo / Base Colour | `ALBEDO` | Diffuse colour map |
| Normal | `NORMAL` | Tangent-space normal map |
| Metallic-Roughness | `METALLIC_ROUGHNESS` | Combined MR texture (ORM) |
| Ambient Occlusion | `AO` | Baked AO map |
| Emissive | `EMISSIVE` | Self-illumination map |
| Height / Displacement | `HEIGHT` | Parallax / tessellation |

Slot definitions: `src/Utilities/Material/MaterialTextureSlot.h`  
Slot enum: `src/Utilities/Material/TextureTypes.h`

---

## Using Materials

```cpp
// Create and configure a material
auto* mat = gameObject->AddComponent<Material>();
mat->albedo    = glm::vec3(0.9f, 0.85f, 0.8f);
mat->metallic  = 0.0f;
mat->roughness = 0.4f;

// Assign textures
auto* texMgr = TextureManager::Instance();
mat->SetTexture(ALBEDO,  texMgr->Load("resources/textures/wall/brickwall.jpg"));
mat->SetTexture(NORMAL,  texMgr->Load("resources/textures/wall/brickwall_normal.jpg"));
```

---

## MaterialManager

`MaterialManager` (singleton) keeps a registry of all loaded materials and provides named lookup.

```cpp
Material* mat = MaterialManager::Instance()->Get("BrickWall");
```

Materials can be serialised and deserialised via `MaterialDto` (see [Serialisation](Serialization.md)).

---

## TextureManager

`TextureManager` (singleton) caches loaded `CustomTexture` objects.  
Textures are uploaded to GPU-resident `VkImage` objects via `TextureManagerModule` (`src/Memory/TextureManagerModule.h`).

```cpp
CustomTexture* tex = TextureManager::Instance()->Load("resources/textures/container2.png");
```

Supported formats:
- **LDR:** PNG, JPG, BMP (via `stb_image`)
- **HDR:** `.hdr` Radiance RGBE (for environment/skybox maps)

---

## ShaderManager

`ShaderManager` (singleton) compiles GLSL shaders to SPIR-V on first use and caches the resulting `VkShaderModule` objects.

```cpp
ShaderModule* vs = ShaderManager::Instance()->Get("resources/shaders/Default/default.vert.spv");
```

Pre-compiled `.spv` files ship with the repository; the manager will load them directly if present.

---

## CustomTexture

`CustomTexture` wraps a `VkImage` + `VkImageView` + `VkSampler` triplet.  
It tracks the image format, mip-level count, and layer count (1 for 2D, 6 for cubemaps).

---

## Material Serialisation

Materials are serialised to YAML through `MaterialDto`:

```yaml
material:
  name: BrickWall
  albedo: [0.9, 0.85, 0.8]
  metallic: 0.0
  roughness: 0.4
  textures:
    albedo: resources/textures/wall/brickwall.jpg
    normal: resources/textures/wall/brickwall_normal.jpg
```

---

## See Also

- [Shader System](Shader-System.md)
- [Rendering Pipeline](Rendering-Pipeline.md)
- [Serialisation](Serialization.md)
