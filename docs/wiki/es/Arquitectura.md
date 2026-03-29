# Descripción de la Arquitectura

> **Idioma:** [English](../en/Architecture.md) · Español  
> ← [Inicio](Inicio.md)

---

## Objetivos de Diseño

Quarantine Engine se construye alrededor de tres principios:

1. **Control de bajo nivel** — uso directo de la API Vulkan sin capas de abstracción intermedias que oculten el comportamiento de la GPU.
2. **Modularidad** — cada subsistema (física, animación, partículas…) es independiente y se comunica a través del modelo de componentes ECS.
3. **Extensibilidad** — se pueden añadir nuevos componentes y sistemas sin modificar el bucle principal del motor.

---

## Organización de Directorios

```
src/
├── App/                   # Controlador de la aplicación (App.h / App.cpp)
├── SetUp/                 # Inicialización de Vulkan (instancia, dispositivo, colas)
├── Draw/                  # Infraestructura de grabación de comandos
├── Presentation/          # Ventana, swapchain, vistas de imagen
├── Memory/                # Memoria GPU, descriptores, UBOs
├── GraphicsPipeline/      # Carga de shaders, objetos de pipeline
├── RayTracing/            # BLAS / TLAS para ray tracing
├── Input/                 # Entrada de teclado y ratón
├── GUI/                   # Integración Dear ImGui
├── Editor/                # Herramientas del editor integrado
├── Data/                  # Serialización, DTOs, gestor de proyectos
├── Utilities/             # Subsistemas de alto nivel del motor (ver abajo)
├── Helpers/               # Utilidades matemáticas y de propósito general
├── Templates/             # Clases de plantilla genéricas
└── main.cpp               # Punto de entrada
```

### Subsistemas de Utilities

```
src/Utilities/
├── General/       # Núcleo ECS: QEGameObject, QEGameComponent, QEScene, QETransform
├── Material/      # Materiales, texturas, shaders
├── Geometry/      # Datos de malla, importación (Assimp), generación de mallas
├── Camera/        # Cámara, frustum culling, spring-arm
├── Light/         # Tipos de luz y gestor de luces
├── Animation/     # Animación esquelética, skinning GPU
├── Physics/       # Wrapper de Jolt Physics, colisionadores, controlador de personaje
├── Particles/     # Sistemas de partículas GPU
├── Atmosphere/    # Renderizado de cielo físicamente basado
├── Compute/       # Nodos de cómputo GPU de propósito general
├── DebugSystem/   # Helpers de dibujo de depuración
└── Controller/    # Controlador de personaje
```

---

## Patrones Principales

### Managers Singleton

Muchos subsistemas del motor exponen un **manager singleton** obtenido vía `::Instance()`:

```cpp
GraphicsPipelineManager::Instance()   // pipelines gráficas
MaterialManager::Instance()           // materiales PBR
LightManager::Instance()              // luces
GameObjectManager::Instance()         // todas las entidades de la escena
PhysicsModule::Instance()             // simulación de física
ParticleSystemManager::Instance()     // emisores de partículas GPU
```

Los singletons se implementan con la plantilla `QESingleton<T>` en `src/Templates/`.

### Entity-Component-System (ECS)

La escena es una colección plana de entidades `QEGameObject`.  
Cada entidad contiene un array de componentes derivados de `QEGameComponent`.  
Los sistemas iteran sobre los componentes durante las pasadas de actualización y dibujo de cada frame.

Consulta [Sistema ECS](Sistema-ECS.md) para más detalles.

### Serialización de Escenas

Las escenas se guardan y cargan desde archivos **YAML** mediante `yaml-cpp`.  
Cada tipo serializable implementa la interfaz `Reflectable` que proporciona los métodos `Serialise` / `Deserialise`.

Consulta [Serialización](Serializacion.md) para más detalles.

---

## Ciclo de Vida de la Aplicación

```
main()
  └─ App::Init()
       ├─ VulkanInstance         (crear instancia Vulkan + capas de validación)
       ├─ DeviceModule           (seleccionar dispositivo físico, crear dispositivo lógico)
       ├─ QueueModule            (obtener colas de gráficos / presentación / cómputo)
       ├─ SwapChainModule        (crear swapchain + vistas de imagen)
       ├─ RenderPassModule       (crear render passes)
       ├─ DepthBufferModule      (attachment de profundidad)
       ├─ AntiAliasingModule     (attachment de resolución MSAA)
       ├─ FrameBufferModule      (framebuffers)
       ├─ CommandPoolModule      (pools de comandos)
       ├─ SynchronizationModule  (semáforos + fences)
       ├─ GraphicsPipelineManager (cargar shaders, crear pipelines)
       ├─ PhysicsModule          (inicializar Jolt)
       └─ QEScene / GameObjectManager  (cargar escena desde YAML)

  App::Run()  (bucle principal)
       ├─ Gestión de entrada     (KeyboardController, eventos GLFW)
       ├─ Paso de física         (PhysicsModule::Update)
       ├─ Actualización animación (Animator / SkeletalComponent)
       ├─ Actualización partículas GPU (dispatch compute de ParticleSystemManager)
       ├─ Frustum culling        (CullingSceneManager)
       ├─ Grabar command buffer  (llamadas vkCmdDraw* por subsistema)
       └─ Presentar frame        (vkQueuePresentKHR)

  App::Cleanup()
       └─ Destruir todos los objetos Vulkan en orden inverso al init
```

---

## Modelo de Hilos

El motor actualmente se ejecuta en un **único hilo principal**.  
El trabajo de GPU se envía de forma asíncrona mediante command buffers de Vulkan y se sincroniza con semáforos y fences (`SynchronizationModule`).

Los compute shaders (skinning de animación, actualización de partículas) se ejecutan de forma concurrente en la cola de cómputo de la GPU.

---

## Arquitectura de Renderizado

El renderer sigue un enfoque **forward+ / forward** con pasadas separadas:

| Pasada | Descripción |
|---|---|
| Shadow Pass | Pasada de profundidad CSM + pasada de profundidad de sombra omnidireccional |
| Geometry Pass | Pasada de shading PBR por defecto (vertex + fragment) |
| Atmosphere Pass | Renderizado de cielo / atmósfera |
| Particle Pass | Partículas GPU transparentes |
| Debug Pass | Wireframes de AABB, gizmos de huesos |
| Grid Pass | Rejilla del editor |
| GUI Pass | Pasada de render de Dear ImGui |

---

## Librerías de Terceros Principales

| Librería | Rol |
|---|---|
| **Vulkan SDK** | API de GPU de bajo nivel |
| **GLFW 3.4** | Creación de ventana, eventos de entrada |
| **GLM 1.0.1** | Matemáticas (vectores, matrices, cuaterniones) |
| **Assimp** | Importación de modelos (OBJ, FBX, GLTF, …) |
| **Jolt Physics** | Simulación de física de cuerpos rígidos |
| **Dear ImGui** (docking) | UI del editor / depuración |
| **SPIRV-Reflect** | Introspección de shaders en tiempo de ejecución |
| **meshoptimizer** | Optimización de mallas y generación de meshlets |
| **stb_image** | Carga de texturas (PNG, JPG, HDR) |
| **yaml-cpp** | Serialización YAML de escenas |

---

## Ver también

- [Sistema ECS](Sistema-ECS.md)
- [Pipeline de Renderizado](Pipeline-Renderizado.md)
- [Serialización](Serializacion.md)
