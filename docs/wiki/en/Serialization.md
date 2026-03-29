# Serialisation

> **Language:** English · [Español](../es/Serializacion.md)  
> ← [Home](Home.md)

---

## Overview

Quarantine Engine uses **YAML** for scene and asset serialisation via the `yaml-cpp` library.  
The serialisation layer lives in `src/Data/` and provides:

- `Reflectable` — base interface for all serialisable types.
- **DTOs** (`src/Data/Dtos/`) — Plain data structs for serialisation round-trips.
- `QEProjectManager` — Manages project files and the active scene file.
- `glm_yaml_conversions.h` — Custom converters for `glm::vec*` / `glm::mat*` / `glm::quat`.

---

## Reflectable Interface

Any type that should be serialisable to / from YAML inherits from `Reflectable`:

```cpp
class Reflectable {
public:
    virtual YAML::Node Serialise()              const = 0;
    virtual void       Deserialise(YAML::Node&)       = 0;
};
```

All major engine types (`QEGameObject`, `Material`, `Light`, `AtmosphereSystem`, …) implement this interface.

---

## Scene Serialisation

### Saving a Scene

```cpp
QEScene* scene = /* active scene */;
scene->Save("QEProjects/MyProject/scenes/main.yaml");
```

### Loading a Scene

```cpp
QEScene* scene = new QEScene();
scene->Load("QEProjects/MyProject/scenes/main.yaml");
```

### Scene YAML Format

```yaml
scene:
  name: MainScene
  objects:
    - id: 1
      name: Player
      transform:
        position: [0.0, 1.0, 0.0]
        rotation: [0.0, 0.0, 0.0, 1.0]   # quaternion xyzw
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
      name: Floor
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

DTOs are lightweight structs in `src/Data/Dtos/` that decouple the serialisation format from internal engine types:

| DTO | File | Serialises |
|---|---|---|
| `GameObjectDto` | `GameObjectDto.h` | `QEGameObject` identity + transform + component list |
| `MaterialDto` | `MaterialDto.h` | `Material` PBR properties + texture paths |
| `LightDto` | `LightDto.h` | Light type, colour, intensity, shadow config |
| `AtmosphereDto` | `AtmosphereDto.h` | Atmosphere parameters |

### Example — MaterialDto

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

## GLM ↔ YAML Conversions

`src/Data/glm_yaml_conversions.h` provides `yaml-cpp` encode/decode specialisations:

```cpp
// vec3
template<> struct convert<glm::vec3> {
    static Node encode(const glm::vec3& v) { ... }
    static bool decode(const Node& node, glm::vec3& v) { ... }
};

// quat, mat4, vec2, vec4 — same pattern
```

This makes `YAML::Node << myVec3` and `myVec3 = node.as<glm::vec3>()` work transparently.

---

## QEProjectManager

`QEProjectManager` manages the on-disk project layout:

```
QEProjects/
└── MyProject/
    ├── project.yaml      ← project metadata
    ├── scenes/
    │   └── main.yaml     ← scene files
    └── assets/           ← (future) project-local asset overrides
```

```cpp
QEProjectManager::Instance()->NewProject("MyProject");
QEProjectManager::Instance()->OpenProject("QEProjects/MyProject/project.yaml");
QEProjectManager::Instance()->SaveCurrentScene();
```

---

## SanitizerHelper

`SanitizerHelper` (`src/Data/SanitizerHelper.h`) validates YAML nodes before deserialisation, logging warnings on missing or mistyped fields without crashing.

---

## See Also

- [ECS System](ECS-System.md)
- [Architecture Overview](Architecture.md)
