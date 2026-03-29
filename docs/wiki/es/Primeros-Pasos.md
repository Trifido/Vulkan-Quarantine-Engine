# Primeros Pasos

> **Idioma:** [English](../en/Getting-Started.md) · Español  
> ← [Inicio](Inicio.md)

---

## Prerrequisitos

Antes de compilar Quarantine Engine, asegúrate de que las siguientes herramientas estén instaladas y accesibles en tu `PATH`.

| Requisito | Versión Mínima | Notas |
|---|---|---|
| **CMake** | 3.16 | Generador de compilación multiplataforma |
| **Vulkan SDK** | 1.3+ | Descarga desde [lunarg.com/vulkan-sdk](https://vulkan.lunarg.com/sdk/home). Establece la variable de entorno `VULKAN_SDK`. |
| **Git** | Cualquier reciente | Necesario para la inicialización de submódulos |
| **Compilador C++** | C++20 | Windows: Visual Studio 2022 (MSVC). Linux: GCC 12 / Clang 14 |
| **Python 3** | 3.x (opcional) | Solo necesario para algunos scripts auxiliares |

> **Windows:** Instala el workload *Desarrollo de escritorio con C++* en Visual Studio 2022.

---

## Clonar el Repositorio

```bash
git clone https://github.com/Trifido/Vulkan-Quarantine-Engine.git
cd Vulkan-Quarantine-Engine
```

### Inicializar los Submódulos

Todas las librerías de terceros están vinculadas como submódulos Git y **deben** inicializarse antes de compilar:

```bash
git submodule update --init --recursive
```

Esto descargará:
- `extern/imgui` – Dear ImGui (rama docking)
- `extern/jolt` – Jolt Physics
- `extern/assimp` – Importador de modelos Assimp
- `extern/SPIRV-Reflect` – Reflexión de shaders SPIR-V
- `extern/meshoptimizer` – Optimizador de mallas

---

## Compilación

### Windows (Visual Studio 2022) — Recomendado

```bash
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release --parallel 8
```

El ejecutable resultante se coloca en:

```
build/Release/QuarantineEngine.exe
```

#### Script de Configuración Automática

Se incluye un script PowerShell para automatizar el paso de configuración de CMake:

```powershell
./setup/build.ps1
```

Este script genera una solución de Visual Studio en `build/` para las configuraciones Debug y Release.

### Windows (compilación Debug)

```bash
cmake --build . --config Debug --parallel 8
# Salida: build/Debug/QuarantineEngine.exe
```

### Linux (GCC / Clang)

> ⚠️ El soporte para Linux está actualmente **en desarrollo**. Algunas características pueden no compilar correctamente.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel $(nproc)
```

---

## Ejecutar el Motor

```bash
# Desde la raíz del repositorio
./build/Release/QuarantineEngine.exe
```

> El motor espera que el directorio `resources/` (shaders, modelos, texturas) sea accesible desde el directorio de trabajo. Lánzalo siempre desde la raíz del proyecto o configura el directorio de trabajo en tu IDE.

---

## Estructura del Proyecto

```
Vulkan-Quarantine-Engine/
├── src/               # Código fuente C++
├── extern/            # Librerías de terceros (submódulos)
├── resources/
│   ├── shaders/       # Código fuente GLSL + SPIR-V precompilado (.spv)
│   ├── models/        # Modelos 3D de ejemplo (OBJ, FBX, GLTF)
│   └── textures/      # Texturas de ejemplo y mapas HDRI de cielo
├── QEProjects/        # Archivos de proyecto / escena guardados
├── setup/             # Scripts de automatización de compilación
├── CMakeLists.txt
└── README.md
```

---

## Configuración en Visual Studio 2022

1. Abre la solución generada `build/QuarantineEngine.sln`.
2. Establece el proyecto de inicio como **QuarantineEngine**.
3. En las propiedades del proyecto → *Depuración*, establece el **Directorio de trabajo** como `$(SolutionDir)../` (la raíz del repositorio).
4. Selecciona **Release x64** o **Debug x64** y pulsa **F5**.

---

## Verificar la Instalación de Vulkan

Ejecuta el siguiente comando para verificar que el Vulkan SDK está instalado correctamente:

```bash
vulkaninfo
```

Si imprime información del dispositivo, está todo listo. Si falla, asegúrate de que:
- `VULKAN_SDK` esté definida (p. ej., `C:\VulkanSDK\1.3.xxx`).
- El driver de la GPU soporta Vulkan 1.3.
- En Windows, `%VULKAN_SDK%\Bin` esté en el `PATH`.

---

## Siguientes Pasos

- [Descripción de la Arquitectura](Arquitectura.md) — aprende cómo está estructurado el motor
- [Sistema ECS](Sistema-ECS.md) — cómo crear game objects y componentes
- [Pipeline de Renderizado](Pipeline-Renderizado.md) — entiende el bucle de dibujo de Vulkan
