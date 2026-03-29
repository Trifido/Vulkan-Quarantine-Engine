# Architecture Overview

> **Language:** English · [Español](../es/Arquitectura.md)  
> ← [Home](Home.md)

---

## High-Level Design Goals

Quarantine Engine is built around three principles:

1. **Low-level control** — direct Vulkan API usage with no intermediary abstraction layers hiding GPU behaviour.
2. **Modularity** — each subsystem (physics, animation, particles, …) is self-contained and communicates through the ECS component model.
3. **Extensibility** — new components and systems can be added without modifying the core engine loop.

---

## Directory Layout

```
src/
├── App/                   # Application controller (App.h / App.cpp)
├── SetUp/                 # Vulkan initialisation (instance, device, queues)
├── Draw/                  # Command recording infrastructure
├── Presentation/          # Window, swapchain, image views
├── Memory/                # GPU memory, descriptors, UBOs
├── GraphicsPipeline/      # Shader loading, pipeline objects
├── RayTracing/            # BLAS / TLAS for ray tracing
├── Input/                 # Keyboard / mouse input
├── GUI/                   # Dear ImGui integration
├── Editor/                # In-editor tools and objects
├── Data/                  # Serialisation, DTOs, project manager
├── Utilities/             # High-level engine subsystems (see below)
├── Helpers/               # Math and general-purpose utilities
├── Templates/             # Generic template classes
└── main.cpp               # Entry point
```

### Utilities Subsystems

```
src/Utilities/
├── General/       # ECS core: QEGameObject, QEGameComponent, QEScene, QETransform
├── Material/      # Materials, textures, shaders
├── Geometry/      # Mesh data, mesh import (Assimp), mesh generation
├── Camera/        # Camera, frustum culling, spring-arm
├── Light/         # Light types and light manager
├── Animation/     # Skeletal animation, GPU skinning
├── Physics/       # Jolt Physics wrapper, colliders, character controller
├── Particles/     # GPU particle systems
├── Atmosphere/    # Physically-based sky rendering
├── Compute/       # General-purpose GPU compute nodes
├── DebugSystem/   # Debug draw helpers
└── Controller/    # Character controller
```

---

## Core Patterns

### Singleton Managers

Many engine subsystems expose a **singleton manager** obtained via `::Instance()`:

```cpp
GraphicsPipelineManager::Instance()   // graphics pipelines
MaterialManager::Instance()           // PBR materials
LightManager::Instance()              // lights
GameObjectManager::Instance()         // all scene entities
PhysicsModule::Instance()             // physics simulation
ParticleSystemManager::Instance()     // GPU particle emitters
```

Singletons are implemented with the `QESingleton<T>` template from `src/Templates/`.

### Entity-Component-System (ECS)

The scene is a flat collection of `QEGameObject` entities.  
Each entity holds an array of `QEGameComponent`-derived components.  
Systems iterate components during the per-frame update and draw passes.

See [ECS System](ECS-System.md) for details.

### Scene Serialisation

Scenes are saved to / loaded from **YAML** files via `yaml-cpp`.  
Every serialisable type implements the `Reflectable` interface which provides `Serialise` / `Deserialise` methods.

See [Serialisation](Serialization.md) for details.

---

## Application Lifecycle

```
main()
  └─ App::Init()
       ├─ VulkanInstance         (create Vulkan instance + validation layers)
       ├─ DeviceModule           (pick physical device, create logical device)
       ├─ QueueModule            (acquire graphics / present / compute queues)
       ├─ SwapChainModule        (create swapchain + image views)
       ├─ RenderPassModule       (create render passes)
       ├─ DepthBufferModule      (depth attachment)
       ├─ AntiAliasingModule     (MSAA resolve attachment)
       ├─ FrameBufferModule      (framebuffers)
       ├─ CommandPoolModule      (command pools)
       ├─ SynchronizationModule  (semaphores + fences)
       ├─ GraphicsPipelineManager (load shaders, create pipelines)
       ├─ PhysicsModule          (initialise Jolt)
       └─ QEScene / GameObjectManager  (load scene from YAML)

  App::Run()  (main loop)
       ├─ Input handling         (KeyboardController, GLFW events)
       ├─ Physics step           (PhysicsModule::Update)
       ├─ Animation update       (Animator / SkeletalComponent)
       ├─ GPU particle update    (ParticleSystemManager compute dispatch)
       ├─ Frustum culling        (CullingSceneManager)
       ├─ Record command buffer  (vkCmdDraw* calls per subsystem)
       └─ Present frame          (vkQueuePresentKHR)

  App::Cleanup()
       └─ Destroy all Vulkan objects in reverse init order
```

---

## Threading Model

The engine currently runs on a **single main thread**.  
GPU work is submitted asynchronously via Vulkan command buffers and synchronised with semaphores and fences (`SynchronizationModule`).

Compute shaders (animation skinning, particle update) run concurrently on the GPU compute queue.

---

## Rendering Architecture

The renderer follows a **forward+ / forward** approach with separate passes:

| Pass | Description |
|---|---|
| Shadow Pass | CSM depth pass + omnidirectional shadow depth pass |
| Geometry Pass | Default PBR shading pass (vertex + fragment) |
| Atmosphere Pass | Sky / atmosphere rendering |
| Particle Pass | Transparent GPU particles |
| Debug Pass | AABB wireframes, bone gizmos |
| Grid Pass | Editor grid overlay |
| GUI Pass | Dear ImGui render pass |

Each pass uses a dedicated `GraphicsPipelineModule` (or `ComputePipelineModule` for compute dispatches).

---

## Key Third-Party Libraries

| Library | Role |
|---|---|
| **Vulkan SDK** | Low-level GPU API |
| **GLFW 3.4** | Window creation, input events |
| **GLM 1.0.1** | Mathematics (vectors, matrices, quaternions) |
| **Assimp** | Model importing (OBJ, FBX, GLTF, …) |
| **Jolt Physics** | Rigid-body physics simulation |
| **Dear ImGui** (docking) | Editor / debug UI |
| **SPIRV-Reflect** | Runtime shader introspection |
| **meshoptimizer** | Mesh optimisation and meshlet generation |
| **stb_image** | Texture loading (PNG, JPG, HDR) |
| **yaml-cpp** | YAML scene serialisation |

---

## See Also

- [ECS System](ECS-System.md)
- [Rendering Pipeline](Rendering-Pipeline.md)
- [Serialisation](Serialization.md)
