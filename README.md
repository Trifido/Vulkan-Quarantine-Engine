<h1 align="center">Quarantine Engine (Vulkan Version)</h1>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0f4c2ab8-577c-46d4-82e8-5894b3a55cb3" alt="Quarantine Engine Logo" width="250"/>
</div>

---

**Quarantine Engine** is a game engine under development built with **C++20** and **Vulkan**.

It started during the 2020 quarantine period with an OpenGL-based prototype and has evolved into a larger engine/editor toolchain focused on modern real-time rendering, asset workflows, and runtime tooling.

[More info about the engine](https://insidethepixels.wordpress.com/2021/01/18/quarantine-engine/)

## Overview

- Low-level Vulkan rendering pipeline
- Custom ECS-style engine architecture
- Scene serialization and deserialization
- Integrated editor built on Dear ImGui
- Runtime project generation and launcher tooling
- Physics, asset import, animation, shader, and material workflows

## Repository Projects

The repository currently contains several code projects and example game projects:

| Path | Project | Type | Description |
| --- | --- | --- | --- |
| `src/QuarantineEngine` | QuarantineEngine | C++ static library | Core engine code: rendering, scene system, physics integration, materials, importers, utilities, and runtime support. |
| `src/QuarantineEditor` | QuarantineEditor | C++ executable | Main editor application built on top of `QuarantineEngine`. Provides the project browser, inspectors, viewport, material/shader tools, and editing workflows. |
| `src/QuarantineLauncher` | Quarantine Launcher | .NET 9 WPF executable | Windows launcher used to open the editor and create/load runtime project templates. |
| `QEProjects/QEExample` | QEExample | Sample project | Minimal example project laid out with `QEAssets` and `QEScenes`. |

## Dependencies

The project mixes **Git submodules**, **CMake FetchContent**, and **toolchain prerequisites**.

### External Libraries

| Dependency | Source | Integration | Purpose |
| --- | --- | --- | --- |
| [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) | System install | `find_package(Vulkan REQUIRED)` | Graphics API, validation layers, headers, and Vulkan linker inputs. |
| [GLFW 3.4](https://github.com/glfw/glfw) | GitHub | CMake `FetchContent` | Window creation, input handling, and Vulkan surface bootstrap. |
| [GLM 1.0.1](https://github.com/g-truc/glm) | GitHub | CMake `FetchContent` | Math library for vectors, matrices, and transforms. |
| [stb](https://github.com/nothings/stb) | GitHub | CMake `FetchContent` | Lightweight single-header utilities, mainly for asset/image related helpers. |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp) | GitHub | CMake `FetchContent` | YAML parsing and serialization for project and asset data. |
| [KTX-Software v4.4.2](https://github.com/KhronosGroup/KTX-Software) | GitHub | CMake `FetchContent` | KTX2 texture handling and Vulkan texture upload support. |
| [Dear ImGui (docking)](https://github.com/ocornut/imgui) | `extern/imgui` | Git submodule + local CMake target | Editor UI framework. |
| [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) | `extern/ImGuizmo` | Vendored source + local CMake target | Gizmos and editor manipulation tools. |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | `extern/jolt` | Git submodule | Physics simulation. |
| [Assimp](https://github.com/assimp/assimp) | `extern/assimp` | Git submodule | Model and scene import pipeline. |
| [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) | `extern/SPIRV-Reflect` | Git submodule + local static library target | Reflection for compiled SPIR-V shaders. |
| [meshoptimizer](https://github.com/zeux/meshoptimizer) | `extern/meshoptimizer` | Git submodule | Mesh optimization and runtime asset processing. |

### Build and Runtime Tooling

| Tool | Required For | Notes |
| --- | --- | --- |
| CMake 3.16+ | Configuring and generating the C++ solution | Main build entry point for engine/editor. |
| Visual Studio 2022 | Windows C++ compilation | Recommended generator: `Visual Studio 17 2022` with x64. |
| Git | Source checkout and submodules | Required for `git submodule update --init --recursive`. |
| .NET SDK 9 | Building `QuarantineLauncher` | Required because the launcher targets `net9.0-windows`. |
| `glslc` | Shader compilation | Detected from `VULKAN_SDK` or a local Vulkan SDK installation by `setup/build.ps1`. |

## Submodules

Initialize the submodules before building:

```bash
git submodule update --init --recursive
```

Current submodule-backed dependencies in this repository:

| Folder | Dependency |
| --- | --- |
| `extern/imgui` | Dear ImGui (`docking` branch) |
| `extern/jolt` | Jolt Physics |
| `extern/assimp` | Assimp |
| `extern/SPIRV-Reflect` | SPIRV-Reflect |
| `extern/meshoptimizer` | meshoptimizer |

## Build Instructions

### Prerequisites

- Vulkan SDK installed and `VULKAN_SDK` available in the environment
- CMake 3.16 or newer
- Git with submodule support
- Visual Studio 2022 with Desktop Development with C++
- .NET 9 SDK for `QuarantineLauncher`

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --parallel 8
```

Generated binaries include:

- `build/Release/QuarantineEditor.exe`
- `src/QuarantineLauncher/bin/.../Quarantine Engine.exe`

## Setup Script

The repository includes a PowerShell helper that:

- cleans and regenerates `build/`
- updates submodules
- compiles shaders through `glslc` when available
- configures CMake for Debug and Release
- builds the engine/editor
- builds the WPF launcher

Usage:

```bash
powershell -ExecutionPolicy Bypass -File .\setup\build.ps1
```

You can also choose a specific configuration:

```bash
powershell -ExecutionPolicy Bypass -File .\setup\build.ps1 -Configuration Debug
powershell -ExecutionPolicy Bypass -File .\setup\build.ps1 -Configuration Release
```

## Screenshots

### Debug Mode
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/e3f6fbe0-0a37-46e2-b860-47258f7fccc9" width="100%"/>

---

### Universal GLTF Converter
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/870372e6-1c7a-44fd-910b-d091a8731b3e" width="100%"/>

---

### GPU Animation System (Compute Shaders)
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c" width="100%"/>

---

### Physics System (Jolt Physics)
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290" width="100%"/>

---

### GPU Particle System
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/0ecb8720-c69c-4157-8656-be98b1800b0c" width="100%"/>

---

### Task/Mesh Shader Pipeline
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/8a8dca40-46b1-4032-809e-947dcba1e77e" width="100%"/>

---
