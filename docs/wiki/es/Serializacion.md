# Serialización

> **Idioma:** [English](../en/Serialization.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Quarantine Engine utiliza **YAML** para la serialización de escenas y activos mediante la librería `yaml-cpp`.  
La capa de serialización reside en `src/Data/` y proporciona:

- `Reflectable` — interfaz base para todos los tipos serializables.
- **DTOs** (`src/Data/Dtos/`) — Estructuras de datos simples para las conversiones de serialización.
- `QEProjectManager` — Gestiona los archivos de proyecto y el archivo de escena activo.
- `glm_yaml_conversions.h` — Conversores personalizados para `glm::vec*` / `glm::mat*` / `glm::quat`.

---

## Interfaz Reflectable

Cualquier tipo que deba ser serializable a / desde YAML hereda de `Reflectable`:

```cpp
class Reflectable {
public:
    virtual YAML::Node Serialise()              const = 0;
    virtual void       Deserialise(YAML::Node&)       = 0;
};
```

Todos los tipos principales del motor (`QEGameObject`, `Material`, `Light`, `AtmosphereSystem`, …) implementan esta interfaz.

---

## Serialización de Escena

### Guardar una Escena

```cpp
QEScene* scene = /* escena activa */;
scene->Save("QEProjects/MiProyecto/scenes/main.yaml");
```

### Cargar una Escena

```cpp
QEScene* scene = new QEScene();
scene->Load("QEProjects/MiProyecto/scenes/main.yaml");
```

### Formato YAML de Escena

```yaml
scene:
  name: MainScene
  objects:
    - id: 1
      name: Jugador
      transform:
        position: [0.0, 1.0, 0.0]
        rotation: [0.0, 0.0, 0.0, 1.0]   # cuaternión xyzw
        scale:    [1.0, 1.0, 1.0]
      components:
        - type: GeometryComponent
          mesh: resources/models/Artorias/Artorias.fbx
        - type: Material
          albedo: [0.8, 0.7, 0.6]
          roughness: 0.4
          metallic: 0.0
          textures:
            albedo: resources/textures/wall/brickwall.jpg
        - type: AnimationComponent
          clip: Idle
          loop: true
    - id: 2
      name: Suelo
      transform:
        position: [0.0, 0.0, 0.0]
        scale:    [20.0, 0.1, 20.0]
      components:
        - type: GeometryComponent
          mesh: _builtin_plane
        - type: PhysicsBody
          motionType: Static
          collider: Plane
```

---

## Data Transfer Objects (DTOs)

Los DTOs son estructuras ligeras en `src/Data/Dtos/` que desacoplan el formato de serialización de los tipos internos del motor:

| DTO | Archivo | Serializa |
|---|---|---|
| `GameObjectDto` | `GameObjectDto.h` | Identidad + transformación + lista de componentes de `QEGameObject` |
| `MaterialDto` | `MaterialDto.h` | Propiedades PBR de `Material` + rutas de textura |
| `LightDto` | `LightDto.h` | Tipo de luz, color, intensidad, configuración de sombras |
| `AtmosphereDto` | `AtmosphereDto.h` | Parámetros de atmósfera |

### Ejemplo — MaterialDto

```cpp
struct MaterialDto {
    std::string name;
    glm::vec3   albedo;
    float       metallic;
    float       roughness;
    float       ambientOcclusion;
    glm::vec3   emissive;
    float       emissiveIntensity;
    std::map<MaterialTextureSlot, std::string> texturePaths;

    YAML::Node Serialise()             const override;
    void       Deserialise(YAML::Node& node) override;
};
```

---

## Conversiones GLM ↔ YAML

`src/Data/glm_yaml_conversions.h` proporciona especializaciones de encode/decode de `yaml-cpp`:

```cpp
// vec3
template<> struct convert<glm::vec3> {
    static Node encode(const glm::vec3& v) { ... }
    static bool decode(const Node& node, glm::vec3& v) { ... }
};

// quat, mat4, vec2, vec4 — mismo patrón
```

Esto hace que `YAML::Node << myVec3` y `myVec3 = node.as<glm::vec3>()` funcionen de forma transparente.

---

## QEProjectManager

`QEProjectManager` gestiona el layout del proyecto en disco:

```
QEProjects/
└── MiProyecto/
    ├── project.yaml      ← metadatos del proyecto
    ├── scenes/
    │   └── main.yaml     ← archivos de escena
    └── assets/           ← (futuro) sobreescrituras de activos locales del proyecto
```

```cpp
QEProjectManager::Instance()->NewProject("MiProyecto");
QEProjectManager::Instance()->OpenProject("QEProjects/MiProyecto/project.yaml");
QEProjectManager::Instance()->SaveCurrentScene();
```

---

## SanitizerHelper

`SanitizerHelper` (`src/Data/SanitizerHelper.h`) valida los nodos YAML antes de la deserialización, registrando advertencias sobre campos faltantes o con tipo incorrecto sin provocar crashes.

---

## Ver también

- [Sistema ECS](Sistema-ECS.md)
- [Descripción de la Arquitectura](Arquitectura.md)
