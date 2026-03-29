# ECS System

> **Language:** English · [Español](../es/Sistema-ECS.md)  
> ← [Home](Home.md)

---

## Overview

Quarantine Engine uses a custom **Entity-Component-System (ECS)** architecture implemented in `src/Utilities/General/`.

| Concept | Class | Role |
|---|---|---|
| **Entity** | `QEGameObject` | Container that holds a list of components |
| **Component** | `QEGameComponent` | Data + behaviour unit attached to an entity |
| **System** | Manager singletons | Iterate specific component types each frame |
| **Scene** | `QEScene` | Owns all game objects; serialisable to YAML |

---

## QEGameObject — the Entity

`QEGameObject` is the central entity class.

```cpp
class QEGameObject {
public:
    std::string name;
    QETransform transform;                          // always present
    std::vector<QEGameComponent*> components;       // optional components
    std::vector<QEGameObject*>    children;         // child objects

    template<typename T> T* AddComponent();
    template<typename T> T* GetComponent();
    template<typename T> void RemoveComponent();

    void AddChild(QEGameObject* child);
    void SetParent(QEGameObject* parent);
};
```

### Transform Hierarchy

Every `QEGameObject` owns a `QETransform` component.  
Transforms can be linked in a parent–child hierarchy, so moving a parent automatically updates all descendants.

```cpp
// Attach an object to a parent
child->SetParent(parent);

// Directly modify local transform
obj->transform.SetPosition({1.0f, 0.0f, 0.0f});
obj->transform.SetRotation(glm::quat{...});
obj->transform.SetScale({2.0f, 2.0f, 2.0f});

// World-space matrix (propagates parent chain)
glm::mat4 world = obj->transform.GetWorldMatrix();
```

---

## QEGameComponent — Base Component

All components inherit from `QEGameComponent`:

```cpp
class QEGameComponent {
public:
    virtual void Update(float deltaTime) {}
    virtual void Render()               {}
    virtual void OnDestroy()            {}
    QEGameObject* gameObject = nullptr; // back-pointer to owning entity
};
```

### Built-in Components

| Component | Header | Purpose |
|---|---|---|
| `QETransform` | `Utilities/General/QETransform.h` | Position, rotation, scale + hierarchy |
| `QEGeometryComponent` | `Utilities/Geometry/QEGeometryComponent.h` | Mesh rendering |
| `Material` | `Utilities/Material/Material.h` | PBR surface material |
| `Light` (subclasses) | `Utilities/Light/Light.h` | Lighting contributions |
| `PhysicsBody` | `Utilities/Physics/PhysicsBody.h` | Rigid-body physics |
| `SkeletalComponent` | `Utilities/Animation/SkeletalComponent.h` | Skeleton hierarchy |
| `QEAnimationComponent` | `Utilities/Animation/QEAnimationComponent.h` | Animation playback |
| `ParticleSystem` | `Utilities/Particles/ParticleSystem.h` | GPU particle emitter |
| `ComputeNode` | `Utilities/Compute/ComputeNode.h` | General GPU compute |
| `AtmosphereSystem` | `Utilities/Atmosphere/AtmosphereSystem.h` | Sky rendering |
| `QECharacterController` | `Utilities/Controller/QECharacterController.h` | Character movement |

---

## QEScene — the Scene Container

`QEScene` owns and manages all `QEGameObject` instances.

```cpp
class QEScene {
public:
    std::string name;
    std::vector<QEGameObject*> rootObjects;  // top-level entities

    void AddGameObject(QEGameObject* obj);
    void RemoveGameObject(QEGameObject* obj);

    void Update(float deltaTime);
    void Render();

    bool Save(const std::string& filepath);
    bool Load(const std::string& filepath);
};
```

### Scene Lifecycle

```
QEScene::Load(path)          // deserialise scene from YAML
    └─ for each object → create QEGameObject + components

App::Run() per frame:
    QEScene::Update(dt)      // tick every component
    QEScene::Render()        // issue draw calls

QEScene::Save(path)          // serialise current state to YAML
```

---

## GameObjectManager

`GameObjectManager` is a singleton that keeps a global registry of all live `QEGameObject` instances (across all scenes):

```cpp
// Retrieve a game object by name
QEGameObject* player = GameObjectManager::Instance()->Find("Player");

// Iterate all objects
for (auto* obj : GameObjectManager::Instance()->GetAll()) { ... }
```

---

## Unique IDs — `Numbered`

Every `QEGameObject` and `QEGameComponent` inherits from `Numbered`, which auto-assigns a monotonically increasing integer ID on construction.  
IDs are used internally for fast lookups and serialisation references.

---

## Session Manager

`QESessionManager` tracks the engine session (project open/close, scene transitions) and is the root lifecycle owner of `QEScene`.

---

## Creating a Game Object (example)

```cpp
// In scene setup code
QEGameObject* box = new QEGameObject("Box");

// Add a mesh
auto* geo = box->AddComponent<QEGeometryComponent>();
geo->SetMesh(MeshImporter::Load("resources/models/box.glb"));

// Add a material
auto* mat = box->AddComponent<Material>();
mat->SetAlbedo(glm::vec3(0.8f, 0.2f, 0.2f));

// Add rigid-body physics
auto* body = box->AddComponent<PhysicsBody>();
body->AddCollider<BoxCollider>(glm::vec3(0.5f)); // half-extents

// Register with the scene
scene->AddGameObject(box);
```

---

## See Also

- [Architecture Overview](Architecture.md)
- [Serialisation](Serialization.md)
- [Physics System](Physics-System.md)
- [Animation System](Animation-System.md)
