<h1 align="center">Quarantine Engine – Documentation Wiki</h1>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0f4c2ab8-577c-46d4-82e8-5894b3a55cb3" alt="Quarantine Engine Logo" width="200"/>
</div>

<div align="center">

**Real-time 3D Game Engine · C++20 · Vulkan**

</div>

---

## 🌐 Choose Language / Elige Idioma

<table>
<tr>
<td width="50%" align="center">

### 🇬🇧 English

| Page | Description |
|------|-------------|
| [🏠 Home](en/Home.md) | Overview, features, screenshots |
| [🚀 Getting Started](en/Getting-Started.md) | Prerequisites, build, first run |
| [🏗️ Architecture](en/Architecture.md) | Project structure, design patterns |
| [🧩 ECS System](en/ECS-System.md) | Entities, components, managers |
| [🎨 Rendering Pipeline](en/Rendering-Pipeline.md) | Vulkan draw loop, render passes |
| [💡 Lighting System](en/Lighting-System.md) | Light types, shadows, CSM |
| [🖌️ Material System](en/Material-System.md) | PBR materials, textures |
| [⚙️ Shader System](en/Shader-System.md) | GLSL shaders, SPIR-V, reflection |
| [🔩 Physics System](en/Physics-System.md) | Jolt Physics, colliders |
| [🦴 Animation System](en/Animation-System.md) | Skeletal animation, GPU skinning |
| [💾 Serialisation](en/Serialization.md) | YAML scenes, DTOs |
| [🤝 Contributing](en/Contributing.md) | Code style, workflow |

</td>
<td width="50%" align="center">

### 🇪🇸 Español

| Página | Descripción |
|--------|-------------|
| [🏠 Inicio](es/Inicio.md) | Resumen, características, capturas |
| [🚀 Primeros Pasos](es/Primeros-Pasos.md) | Prerrequisitos, compilación, primera ejecución |
| [🏗️ Arquitectura](es/Arquitectura.md) | Estructura del proyecto, patrones de diseño |
| [🧩 Sistema ECS](es/Sistema-ECS.md) | Entidades, componentes, managers |
| [🎨 Pipeline Renderizado](es/Pipeline-Renderizado.md) | Bucle de dibujo Vulkan, render passes |
| [💡 Sistema Iluminación](es/Sistema-Iluminacion.md) | Tipos de luz, sombras, CSM |
| [🖌️ Sistema Materiales](es/Sistema-Materiales.md) | Materiales PBR, texturas |
| [⚙️ Sistema Shaders](es/Sistema-Shaders.md) | Shaders GLSL, SPIR-V, reflexión |
| [🔩 Sistema Física](es/Sistema-Fisica.md) | Jolt Physics, colisionadores |
| [🦴 Sistema Animación](es/Sistema-Animacion.md) | Animación esquelética, skinning GPU |
| [💾 Serialización](es/Serializacion.md) | Escenas YAML, DTOs |
| [🤝 Contribuir](es/Contribuir.md) | Estilo de código, flujo de trabajo |

</td>
</tr>
</table>

---

## Quick Start / Inicio Rápido

```bash
# Clone & initialise submodules
git clone https://github.com/Trifido/Vulkan-Quarantine-Engine.git
cd Vulkan-Quarantine-Engine
git submodule update --init --recursive

# Build (Windows)
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --parallel 8

# Run
./build/Release/QuarantineEngine.exe
```

---

## 📄 License

Apache 2.0 — see [`LICENSE`](../../LICENSE)
