# Shader System

> **Language:** English · [Español](../es/Sistema-Shaders.md)  
> ← [Home](Home.md)

---

## Overview

Shaders are written in **GLSL** (version 450 core) and compiled to **SPIR-V** before use.  
The engine loads `.spv` binaries at runtime via `ShaderModule` and introspects binding layouts with **SPIRV-Reflect**.

---

## Shader Directory Layout

```
resources/shaders/
├── Includes/               # Shared GLSL header files
│   ├── QECommon.glsl       # Common types, constants
│   ├── QEBasicLights.glsl  # Light structs and attenuation functions
│   ├── QEMaterial.glsl     # Material UBO struct
│   ├── QEShadows.glsl      # Shadow sampling helpers (PCF, CSM, omni)
│   └── PBR/                # PBR BRDF functions
│       ├── PBR.glsl
│       ├── Distribution.glsl
│       ├── Geometry.glsl
│       └── Fresnel.glsl
│
├── Default/                # Standard PBR forward-rendering pipeline
│   ├── default.vert
│   └── default.frag
│
├── Animation/              # GPU skeletal skinning
│   └── computeSkinning.comp
│
├── Particles/              # GPU particle system
│   ├── emitParticles.comp
│   ├── updateParticles.comp
│   ├── particles.vert
│   └── particles.frag
│
├── Shadow/                 # Shadow depth passes
│   ├── csm.vert            # Cascaded shadow map
│   ├── csm.frag
│   ├── omni_shadow.vert    # Omnidirectional (point) shadow
│   └── omni_shadow.frag
│
├── Mesh/                   # Mesh + task shader pipeline
│   ├── mesh.mesh
│   ├── mesh.task
│   └── mesh.frag
│
├── Atmosphere/             # Sky & atmospheric scattering
│   ├── transmittance_LUT.comp
│   ├── multi_scattering_LUT.comp
│   ├── sky_view_LUT.comp
│   ├── atmosphere.vert
│   ├── atmosphere.frag
│   ├── sky_spherical_map.vert
│   ├── sky_spherical_map.frag
│   ├── skybox_cubemap.vert
│   └── skybox_cubemap.frag
│
├── Compute/                # General-purpose GPU compute
│   └── default_compute.comp
│
├── Debug/                  # Debug visualisation
│   ├── debug.vert
│   ├── debug.frag
│   ├── debugAABB.vert
│   └── debugAABB.frag
│
└── Grid/                   # Editor grid
    ├── grid.vert
    └── grid.frag
```

---

## Compiling Shaders

Pre-compiled `.spv` files are included in the repository.  
To recompile after editing GLSL sources, use the `glslangValidator` or `glslc` tool from the Vulkan SDK:

```bash
# Compile a single shader
glslc resources/shaders/Default/default.vert -o resources/shaders/Default/default_vert.spv
glslc resources/shaders/Default/default.frag -o resources/shaders/Default/default_frag.spv

# Or using glslangValidator
glslangValidator -V resources/shaders/Default/default.vert -o resources/shaders/Default/default_vert.spv
```

---

## ShaderModule — Runtime Loading

`ShaderModule` (`src/GraphicsPipeline/ShaderModule.h`) wraps a `VkShaderModule`:

```cpp
ShaderModule vs("resources/shaders/Default/default_vert.spv",
                VK_SHADER_STAGE_VERTEX_BIT, device);

ShaderModule fs("resources/shaders/Default/default_frag.spv",
                VK_SHADER_STAGE_FRAGMENT_BIT, device);
```

All seven stage types are supported (vertex, fragment, geometry, tessellation control, tessellation evaluation, compute, task, mesh).

---

## ReflectShader — SPIRV-Reflect Integration

`ReflectShader` (`src/GraphicsPipeline/ReflectShader.h`) uses **SPIRV-Reflect** to introspect shader bindings at runtime:

```cpp
ReflectShader reflect(spirvBytes, spirvSize);

// Query descriptor set bindings
for (auto& binding : reflect.GetDescriptorBindings()) {
    // binding.set, binding.binding, binding.descriptor_type …
}
```

This allows the engine to build `VkDescriptorSetLayoutBinding` arrays automatically without hard-coding binding indices.

---

## Shader Includes

GLSL `#include` is supported via `glslc`'s include mechanism.  
Common definitions are in `resources/shaders/Includes/`:

### QECommon.glsl
Defines: `MAX_LIGHTS`, `MAX_CASCADES`, `PI`, common utility macros.

### QEBasicLights.glsl
Structs: `DirectionalLightData`, `PointLightData`, `SpotLightData`.  
Functions: `Attenuation()`, `BlinnPhong()`.

### QEMaterial.glsl
Struct: `MaterialData` (albedo, metallic, roughness, AO, emissive, texture flags).

### QEShadows.glsl
Functions: `SampleCSM()`, `SampleOmniShadow()`, `PCF()`.

---

## GPU Skinning Shader

`computeSkinning.comp` performs skeletal animation on the GPU:

1. Reads bone transformation matrices from an SSBO.
2. Reads rest-pose vertex positions + blend weights from an input SSBO.
3. Writes skinned vertex positions to an output SSBO.
4. The graphics pipeline reads the output SSBO as a vertex buffer.

This avoids CPU-side skinning and keeps the animation cost entirely on the GPU compute queue.

---

## GPU Particle Shaders

| Shader | Stage | Purpose |
|---|---|---|
| `emitParticles.comp` | Compute | Spawns new particles from emitter settings |
| `updateParticles.comp` | Compute | Advances particle positions, applies forces, updates lifetime |
| `particles.vert` | Vertex | Billboard quad expansion |
| `particles.frag` | Fragment | Texture sampling, alpha blend |

---

## Mesh Shader Pipeline

`mesh.task` + `mesh.mesh` + `mesh.frag` implement the modern **task/mesh shader** pipeline:

1. **Task shader** (`mesh.task`): Determines which meshlets to cull and emits a mesh shader workgroup per visible meshlet.
2. **Mesh shader** (`mesh.mesh`): Processes vertices and primitives of a meshlet and outputs them directly to the rasteriser — no vertex buffer binding needed.
3. **Fragment shader** (`mesh.frag`): Standard PBR shading.

Meshlets are generated offline by `meshoptimizer` and stored in GPU-resident SSBOs.

---

## See Also

- [Rendering Pipeline](Rendering-Pipeline.md)
- [Material System](Material-System.md)
- [Animation System](Animation-System.md)
