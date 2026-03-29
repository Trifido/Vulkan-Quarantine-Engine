<h1 align="center">Quarantine Engine – Wiki</h1>

<div align="center">
  <img src="https://github.com/user-attachments/assets/0f4c2ab8-577c-46d4-82e8-5894b3a55cb3" alt="Quarantine Engine Logo" width="200"/>
</div>

> **Idioma:** [English](../en/Home.md) · Español

---

## Bienvenido

**Quarantine Engine** es un motor de juego 3D en tiempo real de código abierto escrito en **C++20** e impulsado por la API gráfica **Vulkan**.  
El proyecto nació durante el período de cuarentena de 2020 como un experimento con OpenGL y ha evolucionado hasta convertirse en un framework gráfico de investigación que abarca renderizado avanzado, cómputo en GPU y arquitectura de motores de juego.

🔗 [Más información en el blog](https://insidethepixels.wordpress.com/2021/01/18/quarantine-engine/)

---

## ✨ Características Principales

| Categoría | Características |
|---|---|
| **Renderizado** | Shading PBR, renderizado deferred/forward, mesh shaders (etapas task + mesh), estructuras de aceleración para ray tracing |
| **Sombras** | Cascaded Shadow Maps (CSM), mapas de sombras omnidireccionales de punto |
| **Atmósfera** | Cielo físicamente basado / scattering atmosférico (LUTs de transmitancia, multi-scattering, sky-view) |
| **Física** | Integración de Jolt Physics – cuerpos rígidos, colisionadores box / sphere / capsule / plane, controlador de personaje |
| **Animación** | Animación esquelética en GPU vía compute shaders de skinning, importación FBX/GLTF con Assimp |
| **Partículas** | Sistema de partículas basado en GPU (compute shaders de emisión y actualización) |
| **ECS** | Entity-Component-System personalizado con jerarquía de escena y transformaciones padre–hijo |
| **Serialización** | Serialización de escena y activos en YAML con carga/guardado completo |
| **Editor UI** | Editor Dear ImGui con ventana de escena, docking, overlay de depuración, rejilla del editor |
| **Entrada** | Gestión de teclado y ratón vía GLFW |

---

## 📚 Páginas de la Wiki

### Primeros Pasos
- [Primeros Pasos](Primeros-Pasos.md) — prerrequisitos, instrucciones de compilación, primera ejecución

### Arquitectura del Motor
- [Descripción de la Arquitectura](Arquitectura.md) — organización de directorios, módulos, patrones de diseño
- [Sistema ECS](Sistema-ECS.md) — entidades, componentes, managers, ciclo de vida de la escena
- [Serialización](Serializacion.md) — escenas YAML, DTOs, gestor de proyectos

### Renderizado y Gráficos
- [Pipeline de Renderizado](Pipeline-Renderizado.md) — inicialización de Vulkan, bucle de dibujo, render passes
- [Sistema de Iluminación](Sistema-Iluminacion.md) — tipos de luz, mapas de sombras, CSM
- [Sistema de Materiales](Sistema-Materiales.md) — materiales PBR, slots de texturas, material manager
- [Sistema de Shaders](Sistema-Shaders.md) — shaders GLSL, compilación SPIR-V, reflexión

### Subsistemas
- [Sistema de Física](Sistema-Fisica.md) — Jolt Physics, colisionadores, physics body
- [Sistema de Animación](Sistema-Animacion.md) — animación esquelética, skinning en GPU

### Contribución
- [Contribuir](Contribuir.md) — estilo de código, flujo de trabajo, cómo añadir un nuevo sistema

---

## 🖼️ Capturas de Pantalla

### Modo Debug
![Debug](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/e3f6fbe0-0a37-46e2-b860-47258f7fccc9)

### Sistema de Animación GPU
![Animation](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c)

### Sistema de Física
![Physics](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290)

### Sistema de Partículas GPU
![Particles](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/0ecb8720-c69c-4157-8656-be98b1800b0c)

### Pipeline Mesh / Task Shader
![Mesh Shaders](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/8a8dca40-46b1-4032-809e-947dcba1e77e)

---

## 📄 Licencia

Quarantine Engine se distribuye bajo la licencia **Apache 2.0**.  
Consulta [`LICENSE`](../../../LICENSE) para el texto completo.
