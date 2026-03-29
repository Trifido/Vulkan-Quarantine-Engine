# Contribuir

> **Idioma:** [English](../en/Contributing.md) · Español  
> ← [Inicio](Inicio.md)

---

## Bienvenido a los Contribuidores

¡Gracias por tu interés en contribuir a Quarantine Engine!  
Esta guía cubre las convenciones de estilo de código, el flujo de trabajo de desarrollo y cómo añadir nuevos sistemas.

---

## Estilo de Código

### Lenguaje y Estándar
- **C++20** en todo el proyecto.
- Usa tipos de la librería estándar (`std::vector`, `std::string`, `std::unique_ptr`, …) en lugar de C puro.
- Evita macros; prefiere funciones `constexpr` / `inline`.

### Convenciones de Nomenclatura

| Construcción | Convención | Ejemplo |
|---|---|---|
| Clases | `PascalCase` | `PhysicsModule`, `QEGameObject` |
| Clases del motor (prefijadas) | `QE` + `PascalCase` | `QEScene`, `QETransform` |
| Métodos / funciones | `PascalCase` | `GetComponent()`, `SetMass()` |
| Variables miembro | `camelCase` | `meshData`, `lightColor` |
| Constantes / enums | `SCREAMING_SNAKE_CASE` | `MAX_LIGHTS`, `EMotionType::Dynamic` |
| Archivos | Coincide con el nombre de la clase | `PhysicsModule.h` / `.cpp` |

### Formato
- Indentación de 4 espacios (ver `.editorconfig`).
- Llave de apertura en la **misma línea** para funciones/métodos.
- Prefiere retornos tempranos (guard clauses) en lugar de anidación profunda.

### Archivos de Cabecera
- Usa `#pragma once` (no include guards).
- Mantén los includes al mínimo; usa declaraciones adelantadas cuando sea posible.
- Separa cabeceras (`.h`) de la implementación (`.cpp`); evita la implementación en cabeceras a menos que sea código de plantilla.

---

## Estructura del Proyecto para Nuevos Sistemas

Al añadir un nuevo subsistema, sigue el patrón existente:

```
src/Utilities/<NuevoSistema>/
├── <NuevoSistema>.h         ← clase de componente o sistema principal
├── <NuevoSistema>.cpp
├── <NuevoSistema>Manager.h  ← manager singleton (si es necesario)
└── <NuevoSistema>Manager.cpp
```

Registra el singleton del manager en el patrón de `src/Templates/QESingleton.h` e inicialízalo en `App::Init()`.

---

## Añadir un Nuevo Componente

1. Crea la clase en `src/Utilities/<Subsistema>/`:

```cpp
// MiComponente.h
#pragma once
#include "QEGameComponent.h"

class MiComponente : public QEGameComponent {
public:
    void Update(float dt) override;
    void Render()         override;

    float miParametro = 1.0f;
};
```

2. Implementa `Update` / `Render` en el archivo `.cpp`.
3. Añade un par `Serialise` / `Deserialise` si el componente debe guardarse en YAML.
4. Añade un DTO correspondiente en `src/Data/Dtos/` si es necesario.
5. Registra el tipo de componente en la factoría de deserialización para que las escenas puedan reconstruirlo.

---

## Añadir un Nuevo Shader

1. Escribe tu shader GLSL en `resources/shaders/<Categoría>/`.
2. Compila a SPIR-V:
   ```bash
   glslc resources/shaders/MiShader/mishader.vert -o resources/shaders/MiShader/mishader_vert.spv
   glslc resources/shaders/MiShader/mishader.frag -o resources/shaders/MiShader/mishader_frag.spv
   ```
3. Crea una instancia de `GraphicsPipelineModule` en `GraphicsPipelineManager` apuntando a los nuevos archivos `.spv`.
4. Usa `ReflectShader` para generar automáticamente los layouts de descriptor, o configúralos manualmente.

---

## Flujo de Trabajo con Git

1. Haz un **fork** del repositorio en GitHub.
2. Crea una rama de característica:
   ```bash
   git checkout -b feature/mi-caracteristica
   ```
3. Haz commits atómicos con descripciones claras:
   ```bash
   git commit -m "Añadir componente CapsuleCollider"
   ```
4. Sube la rama y abre un **Pull Request** contra la rama `main`.
5. Asegúrate de que el proyecto compila limpiamente en Windows (Visual Studio 2022) antes de enviarlo.

---

## Lista de Verificación para Pull Requests

- [ ] El código compila sin advertencias (`/W4` en MSVC).
- [ ] Las nuevas APIs públicas tienen breves comentarios de documentación.
- [ ] Los nuevos tipos serializables tienen DTOs correspondientes y conversión YAML de ida y vuelta.
- [ ] Los shaders compilan a SPIR-V; los archivos `.spv` se incluyen junto al código fuente.
- [ ] Sin rutas absolutas codificadas; usa rutas relativas con `std::filesystem`.
- [ ] Sin nuevas dependencias externas sin discusión previa.

---

## Reportar Errores

Abre un issue en GitHub con:
- Versión del motor / hash de commit.
- Versión de GPU y driver.
- Pasos mínimos de reproducción.
- Log de crash o salida de la capa de validación (habilita las capas de validación de Vulkan en compilaciones Debug).

---

## Ver también

- [Descripción de la Arquitectura](Arquitectura.md)
- [Primeros Pasos](Primeros-Pasos.md)
