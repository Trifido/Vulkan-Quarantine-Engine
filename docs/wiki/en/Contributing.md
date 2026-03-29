# Contributing

> **Language:** English · [Español](../es/Contribuir.md)  
> ← [Home](Home.md)

---

## Welcome Contributors

Thank you for your interest in contributing to Quarantine Engine!  
This guide covers code style conventions, the development workflow, and how to add new systems.

---

## Code Style

### Language & Standard
- **C++20** throughout.
- Use standard-library types (`std::vector`, `std::string`, `std::unique_ptr`, …) over raw C.
- Avoid macros; prefer `constexpr` / `inline` functions.

### Naming Conventions

| Construct | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `PhysicsModule`, `QEGameObject` |
| Engine classes (prefixed) | `QE` + `PascalCase` | `QEScene`, `QETransform` |
| Methods / functions | `PascalCase` | `GetComponent()`, `SetMass()` |
| Member variables | `camelCase` | `meshData`, `lightColor` |
| Constants / enums | `SCREAMING_SNAKE_CASE` | `MAX_LIGHTS`, `EMotionType::Dynamic` |
| Files | Match class name | `PhysicsModule.h` / `.cpp` |

### Formatting
- 4-space indentation (see `.editorconfig`).
- Opening brace on the **same line** for functions/methods.
- Prefer early-return guards over deep nesting.

### Header Files
- Use `#pragma once` (not include guards).
- Keep includes minimal; forward-declare where possible.
- Separate headers (`.h`) from implementation (`.cpp`); avoid implementation in headers unless templated.

---

## Project Structure for New Systems

When adding a new subsystem, follow the existing pattern:

```
src/Utilities/<NewSystem>/
├── <NewSystem>.h         ← main component or system class
├── <NewSystem>.cpp
├── <NewSystem>Manager.h  ← singleton manager (if needed)
└── <NewSystem>Manager.cpp
```

Register the manager singleton in `src/Templates/QESingleton.h` pattern and initialise it in `App::Init()`.

---

## Adding a New Component

1. Create the class in `src/Utilities/<Subsystem>/`:

```cpp
// MyComponent.h
#pragma once
#include "QEGameComponent.h"

class MyComponent : public QEGameComponent {
public:
    void Update(float dt) override;
    void Render()         override;

    float myParam = 1.0f;
};
```

2. Implement `Update` / `Render` in the `.cpp` file.
3. Add a `Serialise` / `Deserialise` pair if the component should be saved to YAML.
4. Add a corresponding DTO in `src/Data/Dtos/` if needed.
5. Register the component type in the deserialisation factory so scenes can reconstruct it.

---

## Adding a New Shader

1. Write your GLSL shader in `resources/shaders/<Category>/`.
2. Compile to SPIR-V:
   ```bash
   glslc resources/shaders/MyShader/myshader.vert -o resources/shaders/MyShader/myshader_vert.spv
   glslc resources/shaders/MyShader/myshader.frag -o resources/shaders/MyShader/myshader_frag.spv
   ```
3. Create a `GraphicsPipelineModule` instance in `GraphicsPipelineManager` pointing to the new `.spv` files.
4. Use `ReflectShader` to auto-generate descriptor layouts, or set them manually.

---

## Git Workflow

1. **Fork** the repository on GitHub.
2. Create a feature branch:
   ```bash
   git checkout -b feature/my-feature
   ```
3. Commit atomic, well-described changes:
   ```bash
   git commit -m "Add CapsuleCollider component"
   ```
4. Push and open a **Pull Request** against the `main` branch.
5. Ensure the project builds cleanly on Windows (Visual Studio 2022) before submitting.

---

## Pull Request Checklist

- [ ] Code compiles without warnings (`/W4` on MSVC).
- [ ] New public API has brief doc-comments.
- [ ] New serialisable types have matching DTOs and YAML round-trip.
- [ ] Shaders compile to SPIR-V; `.spv` files are committed alongside source.
- [ ] No hard-coded absolute paths; use `std::filesystem` relative paths.
- [ ] No new external dependencies without discussion.

---

## Reporting Bugs

Open an issue on GitHub with:
- Engine version / commit hash.
- GPU and driver version.
- Minimal reproduction steps.
- Crash log or validation-layer output (enable Vulkan validation layers in debug builds).

---

## See Also

- [Architecture Overview](Architecture.md)
- [Getting Started](Getting-Started.md)
