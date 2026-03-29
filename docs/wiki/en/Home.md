<h1 align="center">Quarantine Engine – Wiki</h1>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0f4c2ab8-577c-46d4-82e8-5894b3a55cb3" alt="Quarantine Engine Logo" width="200"/>
</div>

> **Language:** English · [Español](../es/Inicio.md)

---

## Welcome

**Quarantine Engine** is an open-source, real-time 3D game engine written in **C++20** and powered by the **Vulkan** graphics API.  
The project was born during the 2020 quarantine period as an OpenGL experiment and has since evolved into a research-grade graphics framework covering advanced rendering, GPU compute, and game-engine architecture.

🔗 [More info on the blog](https://insidethepixels.wordpress.com/2021/01/18/quarantine-engine/)

---

## ✨ Feature Highlights

| Category | Features |
|---|---|
| **Rendering** | PBR shading, deferred/forward rendering, mesh shaders (task + mesh stages), ray tracing acceleration structures |
| **Shadows** | Cascaded Shadow Maps (CSM), omnidirectional point-shadow maps |
| **Atmosphere** | Physically-based sky / atmospheric scattering (transmittance, multi-scatter, sky-view LUTs) |
| **Physics** | Jolt Physics integration – rigid bodies, box / sphere / capsule / plane colliders, character controller |
| **Animation** | GPU skeletal animation via compute skinning shaders, FBX/GLTF import through Assimp |
| **Particles** | GPU-based particle system (emit + update compute shaders) |
| **ECS** | Custom Entity-Component-System with scene hierarchy and parent–child transforms |
| **Serialization** | YAML-based scene & asset serialization with full round-trip load/save |
| **Editor UI** | Dear ImGui editor with scene window, docking, debug overlay, editor grid |
| **Input** | Keyboard / mouse input via GLFW |

---

## 📚 Wiki Pages

### Getting Started
- [Getting Started](Getting-Started.md) — prerequisites, build instructions, first run

### Engine Architecture
- [Architecture Overview](Architecture.md) — directory layout, module boundaries, design patterns
- [ECS System](ECS-System.md) — entities, components, managers, scene lifecycle
- [Serialization](Serialization.md) — YAML scenes, DTOs, project manager

### Rendering & Graphics
- [Rendering Pipeline](Rendering-Pipeline.md) — Vulkan initialisation, draw loop, render passes
- [Lighting System](Lighting-System.md) — light types, shadow maps, CSM
- [Material System](Material-System.md) — PBR materials, texture slots, material manager
- [Shader System](Shader-System.md) — GLSL shaders, SPIR-V compilation, reflection

### Subsystems
- [Physics System](Physics-System.md) — Jolt Physics, colliders, physics body
- [Animation System](Animation-System.md) — skeletal animation, GPU skinning

### Contributing
- [Contributing](Contributing.md) — code style, workflow, how to add a new system

---

## 🖼️ Screenshots

### Debug Mode
![Debug](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/e3f6fbe0-0a37-46e2-b860-47258f7fccc9)

### GPU Animation System
![Animation](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c)

### Physics System
![Physics](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290)

### GPU Particle System
![Particles](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/0ecb8720-c69c-4157-8656-be98b1800b0c)

### Mesh / Task Shader Pipeline
![Mesh Shaders](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/8a8dca40-46b1-4032-809e-947dcba1e77e)

---

## 📄 License

Quarantine Engine is released under the **Apache 2.0** license.  
See [`LICENSE`](../../../LICENSE) for the full text.
