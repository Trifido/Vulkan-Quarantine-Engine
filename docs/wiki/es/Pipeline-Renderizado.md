# Pipeline de Renderizado

> **Idioma:** [English](../en/Rendering-Pipeline.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Quarantine Engine utiliza la **API Vulkan** directamente.  
El bucle de renderizado realiza sincronización explícita, grabación de command buffers y envío a cola en cada frame.

---

## Secuencia de Inicialización de Vulkan

```
VulkanInstance           → crear VkInstance, habilitar capas de validación
VulkanLayerAndExtension  → enumerar capas/extensiones disponibles
DeviceModule             → seleccionar VkPhysicalDevice, crear VkDevice
QueueModule              → obtener VkQueues de gráficos / presentación / cómputo
WindowSurface            → crear VkSurfaceKHR vía GLFW
SwapChainModule          → crear VkSwapchainKHR (doble / triple buffer)
ImageViewModule          → VkImageView por imagen de swapchain
DepthBufferModule        → VkImage + VkImageView de profundidad (D32_SFLOAT)
AntiAliasingModule       → attachment de resolución MSAA (VkSampleCountFlagBits)
RenderPassModule         → VkRenderPass con attachments de color + profundidad + resolución
FrameBufferModule        → un VkFramebuffer por imagen de swapchain
CommandPoolModule        → VkCommandPool + VkCommandBuffer por frame
SynchronizationModule    → VkSemaphore (image-available, render-finished) + VkFence
```

---

## Bucle de Renderizado por Frame

```cpp
// Pseudocódigo que refleja App::DrawFrame()
vkWaitForFences(inFlightFence);                          // 1. esperar frame anterior
vkAcquireNextImageKHR(swapchain, imageAvailableSem);     // 2. adquirir imagen

vkResetCommandBuffer(commandBuffer);
vkBeginCommandBuffer(commandBuffer);
  RecordShadowPass(commandBuffer);                       // 3a. pasadas de profundidad de sombra
  RecordMainRenderPass(commandBuffer);                   // 3b. geometría PBR
  RecordComputeDispatches(commandBuffer);                // 3c. skinning / partículas
  RecordAtmospherePass(commandBuffer);                   // 3d. cielo
  RecordParticlePass(commandBuffer);                     // 3e. partículas transparentes
  RecordDebugPass(commandBuffer);                        // 3f. overlays de depuración
  RecordEditorGUI(commandBuffer);                        // 3g. ImGui
vkEndCommandBuffer(commandBuffer);

vkQueueSubmit(graphicsQueue, commandBuffer,              // 4. enviar
              waitSem=imageAvailableSem,
              signalSem=renderFinishedSem,
              fence=inFlightFence);

vkQueuePresentKHR(presentQueue, renderFinishedSem);      // 5. presentar
```

---

## Render Passes

### Shadow Pass

Dos sub-pasadas de sombra se ejecutan antes de la pasada de geometría principal:

| Sub-pasada | Clase | Salida |
|---|---|---|
| Cascaded Shadow Map (CSM) | `ShadowPipelineManager` + `CSMResources` | Imagen array de profundidad (N cascadas) |
| Sombra Omnidireccional | `ShadowPipelineManager` + `OmniShadowResources` | Cubemap de profundidad por luz puntual |

Las divisiones de cascada siguen una distribución logarítmica.  
La pasada principal muestrea los mapas de sombra usando filtrado PCF (vía `QEShadows.glsl`).

### Main Geometry Pass

Dibuja todos los objetos `QEGeometryComponent` opacos con el pipeline PBR por defecto:

- **Vertex shader:** `resources/shaders/Default/default.vert` — transforma vértices, calcula la matriz TBN.
- **Fragment shader:** `resources/shaders/Default/default.frag` — BRDF PBR, muestreo de sombras, scattering atmosférico.

### Mesh Shader Pass (opcional)

Cuando la extensión `VK_EXT_mesh_shader` está disponible, un pipeline de task + mesh shader (`resources/shaders/Mesh/`) puede reemplazar el pipeline de vértices.  
Los meshlets son generados por `meshoptimizer` y almacenados en estructuras `Meshlet`.

### Atmosphere Pass

Renderiza el domo de cielo usando tablas de consulta (LUTs) precalculadas:

| LUT | Compute Shader |
|---|---|
| Transmittance LUT | `transmittance_LUT.comp` |
| Multi-scattering LUT | `multi_scattering_LUT.comp` |
| Sky-view LUT | `sky_view_LUT.comp` |

El cielo final se dibuja por `atmosphere.vert` / `atmosphere.frag` o `sky_spherical_map.*`.

### Debug / Editor Passes

| Pasada | Shader | Propósito |
|---|---|---|
| Wireframe AABB | `debugAABB.vert/frag` | Contornos de colisionadores de física |
| Líneas de depuración | `debug.vert/frag` | Gizmos de huesos, frustum de cámara |
| Rejilla | `grid.vert/frag` | Rejilla de suelo del editor |
| ImGui | Backend Vulkan de Dear ImGui | UI del editor |

---

## Pipelines Gráficas

`GraphicsPipelineManager` (singleton) posee todas las instancias de `GraphicsPipelineModule`.  
Cada módulo encapsula un `VkPipeline` + `VkPipelineLayout` + `VkDescriptorSetLayout`.

```cpp
// Recuperar un pipeline por nombre
auto* pipeline = GraphicsPipelineManager::Instance()->Get("default_pbr");

// Vincular y dibujar
pipeline->Bind(commandBuffer);
vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
```

### Etapas de Shader Soportadas

| Etapa | Flag de Etapa Vulkan |
|---|---|
| Vértice | `VK_SHADER_STAGE_VERTEX_BIT` |
| Fragmento | `VK_SHADER_STAGE_FRAGMENT_BIT` |
| Geometría | `VK_SHADER_STAGE_GEOMETRY_BIT` |
| Control de Teselación | `VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT` |
| Evaluación de Teselación | `VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT` |
| Cómputo | `VK_SHADER_STAGE_COMPUTE_BIT` |
| Task (EXT) | `VK_SHADER_STAGE_TASK_BIT_EXT` |
| Mesh (EXT) | `VK_SHADER_STAGE_MESH_BIT_EXT` |

---

## Gestión de Descriptores

Los descriptores se gestionan por pasada:

| Clase | Propósito |
|---|---|
| `DescriptorBuffer` | Descriptores UBO / sampler por objeto |
| `ComputeDescriptorBuffer` | Storage buffers para compute shaders |
| `CSMDescriptorsManager` | Descriptores de sampler para el array de sombras en cascada |
| `PointShadowDescriptorsManager` | Descriptores de cubemap para sombras omnidireccionales |

---

## Uniform Buffer Objects

Las estructuras UBO se definen en `src/Memory/UBO.h` y se actualizan una vez por frame:

```cpp
struct ViewProjectionUBO {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec3 cameraPosition;
};

struct ModelUBO {
    glm::mat4 model;
    glm::mat4 normalMatrix;
};
```

---

## Multi-Sampling (MSAA)

El motor soporta MSAA hasta el máximo del hardware.  
El número de muestras se consulta desde `VkPhysicalDeviceProperties` y se configura en `AntiAliasingModule`.  
La resolución MSAA se realiza como parte del attachment de resolución de la render pass principal.

---

## Ray Tracing

`RayTracingModule` construye **Bottom-Level Acceleration Structures (BLAS)** por malla y una **Top-Level Acceleration Structure (TLAS)** para la escena.  
Se utiliza la extensión `VK_KHR_acceleration_structure`.  
El shading completo por ray tracing es un trabajo futuro; la infraestructura está lista para renderizado híbrido.

---

## Ver también

- [Sistema de Shaders](Sistema-Shaders.md)
- [Sistema de Iluminación](Sistema-Iluminacion.md)
- [Sistema de Materiales](Sistema-Materiales.md)
