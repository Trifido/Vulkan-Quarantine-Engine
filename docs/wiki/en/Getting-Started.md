# Getting Started

> **Language:** English · [Español](../es/Primeros-Pasos.md)  
> ← [Home](Home.md)

---

## Prerequisites

Before building Quarantine Engine make sure the following tools are installed and accessible on your `PATH`.

| Requirement | Minimum Version | Notes |
|---|---|---|
| **CMake** | 3.16 | Cross-platform build generator |
| **Vulkan SDK** | 1.3+ | Download from [lunarg.com/vulkan-sdk](https://vulkan.lunarg.com/sdk/home). Set `VULKAN_SDK` environment variable. |
| **Git** | Any recent | Required for submodule initialisation |
| **C++ Compiler** | C++20 | Windows: Visual Studio 2022 (MSVC). Linux: GCC 12 / Clang 14 |
| **Python 3** | 3.x (optional) | Only needed for some helper scripts |

> **Windows:** Install the *Desktop Development with C++* workload in Visual Studio 2022.

---

## Cloning the Repository

```bash
git clone https://github.com/Trifido/Vulkan-Quarantine-Engine.git
cd Vulkan-Quarantine-Engine
```

### Initialise Submodules

All third-party libraries are tracked as Git submodules and **must** be initialised before building:

```bash
git submodule update --init --recursive
```

This will fetch:
- `extern/imgui` – Dear ImGui (docking branch)
- `extern/jolt` – Jolt Physics
- `extern/assimp` – Assimp model importer
- `extern/SPIRV-Reflect` – SPIR-V shader reflection
- `extern/meshoptimizer` – Mesh optimiser

---

## Building

### Windows (Visual Studio 2022) — Recommended

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --parallel 8
```

The resulting executable is placed at:

```
build/Release/QuarantineEngine.exe
```

#### Automated Setup Script

A PowerShell script is provided to automate the CMake configuration step:

```powershell
./setup/build.ps1
```

This generates a Visual Studio solution under `build/` for both Debug and Release configurations.

### Windows (Debug build)

```bash
cmake --build . --config Debug --parallel 8
# Output: build/Debug/QuarantineEngine.exe
```

### Linux (GCC / Clang)

> ⚠️ Linux support is currently **in progress**. Some features may not compile out-of-the-box.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel $(nproc)
```

---

## Running the Engine

```bash
# From the repository root
./build/Release/QuarantineEngine.exe
```

> The engine expects the `resources/` directory (shaders, models, textures) to be reachable relative to the working directory. Always launch from the project root or set the working directory accordingly in your IDE.

---

## Project Structure at a Glance

```
Vulkan-Quarantine-Engine/
├── src/               # C++ source code
├── extern/            # Third-party libraries (submodules)
├── resources/
│   ├── shaders/       # GLSL source + pre-compiled SPIR-V (.spv)
│   ├── models/        # Sample 3D models (OBJ, FBX, GLTF)
│   └── textures/      # Sample textures and HDRI sky maps
├── QEProjects/        # Saved project / scene files
├── setup/             # Build automation scripts
├── CMakeLists.txt
└── README.md
```

---

## IDE Setup (Visual Studio 2022)

1. Open the generated `build/QuarantineEngine.sln`.
2. Set the startup project to **QuarantineEngine**.
3. In project properties → *Debugging*, set **Working Directory** to `$(SolutionDir)../` (the repository root).
4. Select **Release x64** or **Debug x64** and press **F5**.

---

## Verifying the Vulkan Installation

Run the following command to verify the Vulkan SDK is correctly installed:

```bash
vulkaninfo
```

If it prints device information, you're good to go. If it fails, ensure:
- `VULKAN_SDK` is set (e.g., `C:\VulkanSDK\1.3.xxx`).
- The GPU driver supports Vulkan 1.3.
- On Windows, `%VULKAN_SDK%\Bin` is on `PATH`.

---

## Next Steps

- [Architecture Overview](Architecture.md) — learn how the engine is structured
- [ECS System](ECS-System.md) — how to create game objects and components
- [Rendering Pipeline](Rendering-Pipeline.md) — understand the Vulkan draw loop
