<h1 align="center">Quarantine Engine (Vulkan Version)</h1>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0f4c2ab8-577c-46d4-82e8-5894b3a55cb3" alt="Quarantine Engine Logo" width="250"/>
</div>

---

**Quarantine Engine** is a game engine under development using **C++** and **Vulkan**.

Started during the 2020 quarantine period with **OpenGL**, it has since evolved into a more sophisticated project aiming to explore and implement advanced techniques in real-time 3D graphics.

🔗 [More info about the engine](https://insidethepixels.wordpress.com/2021/01/18/quarantine-engine/)


## 🎓 Learning Objectives

- Master low-level Vulkan API
- Implement real-time graphics techniques
- Explore GPU compute workflows
- Learn modular C++ engine design

---

## 🚀 Features

- Low-level Vulkan rendering pipeline
- Custom ECS (Entity Component System)
- Scene serialization and deserialization
- Cross-platform architecture in progress (Windows/Linux)
- Modular code structure using modern C++

---


## 📦 Dependencies (Submodules)

Before building, initialize and update all submodules:

```bash
git submodule update --init --recursive
```

| Folder                 | Description                                                               | Branch / Tag |
| ---------------------- | ------------------------------------------------------------------------- | ------------ |
| `extern/imgui`         | [Dear ImGui](https://github.com/ocornut/imgui) with docking support       | `docking`    |
| `extern/glfw`          | [GLFW 3](https://github.com/glfw/glfw)                                    | `3.4`        |
| `extern/glm`           | [GLM](https://github.com/g-truc/glm)                                      | `1.0.1`      |
| `extern/stb`           | [stb single-file headers](https://github.com/nothings/stb)                | `master`     |
| `extern/jolt`          | [Jolt Physics](https://github.com/jrouwe/JoltPhysics)                     | latest       |
| `extern/assimp`        | [Assimp](https://github.com/assimp/assimp)                                | latest       |
| `extern/SPIRV-Reflect` | [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect)            | latest       |
| `extern/meshoptimizer` | [meshoptimizer](https://github.com/zeux/meshoptimizer)                    | latest       |

## 🛠️ Build Instructions
### Prerequisites

- CMake ≥ 3.16
- Vulkan SDK installed and VULKAN_SDK environment variable set
- Git with submodule support
- On Windows: Visual Studio 2022 (Desktop Development with C++)

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --parallel 8
```
- Executable: build/Release/QuarantineEngine.exe

## ⚙️ Setup Scripts
We provide a PowerShell script to automate project generation:
```bash
./setup/build.ps1
```
Running this script will configure and generate the Visual Studio solution (x64, Debug & Release) under build/.

## 📸 Screenshots

### 🛠️ Debug Mode
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/e3f6fbe0-0a37-46e2-b860-47258f7fccc9" width="100%"/>

---

### 📦 Universal GLTF Converter
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/870372e6-1c7a-44fd-910b-d091a8731b3e" width="100%"/>

---

### 🦴 GPU Animation System *(Compute Shaders)*
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c" width="100%"/>

---

### 🎯 Physics System ([Jolt Physics](https://github.com/jrouwe/JoltPhysics))
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290" width="100%"/>

---

### 🌪 GPU Particle System
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/0ecb8720-c69c-4157-8656-be98b1800b0c" width="100%"/>

---

### 🧩 Task/Mesh Shader Pipeline
<img src="https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/8a8dca40-46b1-4032-809e-947dcba1e77e" width="100%"/>

---
