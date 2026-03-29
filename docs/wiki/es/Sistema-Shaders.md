# Sistema de Shaders

> **Idioma:** [English](../en/Shader-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Los shaders están escritos en **GLSL** (versión 450 core) y compilados a **SPIR-V** antes de su uso.  
El motor carga binarios `.spv` en tiempo de ejecución mediante `ShaderModule` e introspecciona los layouts de binding con **SPIRV-Reflect**.

---

## Organización del Directorio de Shaders

```
resources/shaders/
├── Includes/               # Archivos de cabecera GLSL compartidos
│   ├── QECommon.glsl       # Tipos comunes, constantes
│   ├── QEBasicLights.glsl  # Estructuras de luz y funciones de atenuación
│   ├── QEMaterial.glsl     # Estructura UBO de material
│   ├── QEShadows.glsl      # Helpers de muestreo de sombras (PCF, CSM, omni)
│   └── PBR/                # Funciones BRDF PBR
│       ├── PBR.glsl
│       ├── Distribution.glsl
│       ├── Geometry.glsl
│       └── Fresnel.glsl
│
├── Default/                # Pipeline forward de renderizado PBR estándar
│   ├── default.vert
│   └── default.frag
│
├── Animation/              # Skinning esquelético GPU
│   └── computeSkinning.comp
│
├── Particles/              # Sistema de partículas GPU
│   ├── emitParticles.comp
│   ├── updateParticles.comp
│   ├── particles.vert
│   └── particles.frag
│
├── Shadow/                 # Pasadas de profundidad de sombra
│   ├── csm.vert            # Cascaded shadow map
│   ├── csm.frag
│   ├── omni_shadow.vert    # Sombra omnidireccional (puntual)
│   └── omni_shadow.frag
│
├── Mesh/                   # Pipeline mesh + task shader
│   ├── mesh.mesh
│   ├── mesh.task
│   └── mesh.frag
│
├── Atmosphere/             # Cielo y scattering atmosférico
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
├── Compute/                # Cómputo GPU de propósito general
│   └── default_compute.comp
│
├── Debug/                  # Visualización de depuración
│   ├── debug.vert
│   ├── debug.frag
│   ├── debugAABB.vert
│   └── debugAABB.frag
│
└── Grid/                   # Rejilla del editor
    ├── grid.vert
    └── grid.frag
```

---

## Compilar Shaders

Los archivos `.spv` precompilados se incluyen en el repositorio.  
Para recompilar tras editar las fuentes GLSL, usa la herramienta `glslangValidator` o `glslc` del Vulkan SDK:

```bash
# Compilar un shader individual
glslc resources/shaders/Default/default.vert -o resources/shaders/Default/default_vert.spv
glslc resources/shaders/Default/default.frag -o resources/shaders/Default/default_frag.spv

# O usando glslangValidator
glslangValidator -V resources/shaders/Default/default.vert -o resources/shaders/Default/default_vert.spv
```

---

## ShaderModule — Carga en Tiempo de Ejecución

`ShaderModule` (`src/GraphicsPipeline/ShaderModule.h`) encapsula un `VkShaderModule`:

```cpp
ShaderModule vs("resources/shaders/Default/default_vert.spv",
                VK_SHADER_STAGE_VERTEX_BIT, device);

ShaderModule fs("resources/shaders/Default/default_frag.spv",
                VK_SHADER_STAGE_FRAGMENT_BIT, device);
```

Los siete tipos de etapa están soportados (vertex, fragment, geometry, tessellation control, tessellation evaluation, compute, task, mesh).

---

## ReflectShader — Integración SPIRV-Reflect

`ReflectShader` (`src/GraphicsPipeline/ReflectShader.h`) usa **SPIRV-Reflect** para introspeccionar bindings de shaders en tiempo de ejecución:

```cpp
ReflectShader reflect(spirvBytes, spirvSize);

// Consultar bindings de descriptor sets
for (auto& binding : reflect.GetDescriptorBindings()) {
    // binding.set, binding.binding, binding.descriptor_type …
}
```

Esto permite al motor construir arrays de `VkDescriptorSetLayoutBinding` automáticamente sin codificar índices de binding.

---

## Includes de Shader

`#include` en GLSL está soportado mediante el mecanismo de include de `glslc`.  
Las definiciones comunes están en `resources/shaders/Includes/`:

### QECommon.glsl
Define: `MAX_LIGHTS`, `MAX_CASCADES`, `PI`, macros utilitarios comunes.

### QEBasicLights.glsl
Structs: `DirectionalLightData`, `PointLightData`, `SpotLightData`.  
Funciones: `Attenuation()`, `BlinnPhong()`.

### QEMaterial.glsl
Struct: `MaterialData` (albedo, metallic, roughness, AO, emissive, texture flags).

### QEShadows.glsl
Funciones: `SampleCSM()`, `SampleOmniShadow()`, `PCF()`.

---

## Shader de Skinning GPU

`computeSkinning.comp` realiza la animación esquelética en la GPU:

1. Lee las matrices de transformación de huesos desde un SSBO.
2. Lee posiciones de vértices en pose en reposo + pesos de blend desde un SSBO de entrada.
3. Escribe posiciones de vértices con skinning en un SSBO de salida.
4. El pipeline gráfico lee el SSBO de salida como vertex buffer.

Esto evita el skinning en CPU y mantiene el costo de animación completamente en la cola de cómputo de la GPU.

---

## Shaders de Partículas GPU

| Shader | Etapa | Propósito |
|---|---|---|
| `emitParticles.comp` | Compute | Genera nuevas partículas desde la configuración del emisor |
| `updateParticles.comp` | Compute | Avanza posiciones de partículas, aplica fuerzas, actualiza tiempo de vida |
| `particles.vert` | Vertex | Expansión de quad billboard |
| `particles.frag` | Fragment | Muestreo de textura, mezcla alfa |

---

## Pipeline Mesh Shader

`mesh.task` + `mesh.mesh` + `mesh.frag` implementan el moderno pipeline de **task/mesh shader**:

1. **Task shader** (`mesh.task`): Determina qué meshlets eliminar y emite un grupo de trabajo de mesh shader por meshlet visible.
2. **Mesh shader** (`mesh.mesh`): Procesa vértices y primitivas de un meshlet y los envía directamente al rasterizador — sin necesidad de vincular vertex buffer.
3. **Fragment shader** (`mesh.frag`): Shading PBR estándar.

Los meshlets se generan offline por `meshoptimizer` y se almacenan en SSBOs residentes en GPU.

---

## Ver también

- [Pipeline de Renderizado](Pipeline-Renderizado.md)
- [Sistema de Materiales](Sistema-Materiales.md)
- [Sistema de Animación](Sistema-Animacion.md)
