# Sistema ECS

> **Idioma:** [English](../en/ECS-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Quarantine Engine utiliza una arquitectura personalizada de **Entity-Component-System (ECS)** implementada en `src/Utilities/General/`.

| Concepto | Clase | Rol |
|---|---|---|
| **Entidad** | `QEGameObject` | Contenedor que alberga una lista de componentes |
| **Componente** | `QEGameComponent` | Unidad de datos + comportamiento adjunta a una entidad |
| **Sistema** | Managers singleton | Iteran tipos de componentes específicos en cada frame |
| **Escena** | `QEScene` | Posee todos los game objects; serializable a YAML |

---

## QEGameObject — la Entidad

`QEGameObject` es la clase central de entidad.

```cpp
class QEGameObject {
public:
    std::string name;
    QETransform transform;                          // siempre presente
    std::vector<QEGameComponent*> components;       // componentes opcionales
    std::vector<QEGameObject*>    children;         // objetos hijo

    template<typename T> T* AddComponent();
    template<typename T> T* GetComponent();
    template<typename T> void RemoveComponent();

    void AddChild(QEGameObject* child);
    void SetParent(QEGameObject* parent);
};
```

### Jerarquía de Transformaciones

Cada `QEGameObject` posee un componente `QETransform`.  
Las transformaciones pueden vincularse en una jerarquía padre–hijo, de modo que mover un padre actualiza automáticamente todos sus descendientes.

```cpp
// Asociar un objeto a un padre
child->SetParent(parent);

// Modificar la transformación local directamente
obj->transform.SetPosition({1.0f, 0.0f, 0.0f});
obj->transform.SetRotation(glm::quat{...});
obj->transform.SetScale({2.0f, 2.0f, 2.0f});

// Matriz en espacio global (propaga la cadena de padres)
glm::mat4 world = obj->transform.GetWorldMatrix();
```

---

## QEGameComponent — Componente Base

Todos los componentes heredan de `QEGameComponent`:

```cpp
class QEGameComponent {
public:
    virtual void Update(float deltaTime) {}
    virtual void Render()               {}
    virtual void OnDestroy()            {}
    QEGameObject* gameObject = nullptr; // puntero de retorno a la entidad propietaria
};
```

### Componentes Integrados

| Componente | Cabecera | Propósito |
|---|---|---|
| `QETransform` | `Utilities/General/QETransform.h` | Posición, rotación, escala + jerarquía |
| `QEGeometryComponent` | `Utilities/Geometry/QEGeometryComponent.h` | Renderizado de mallas |
| `Material` | `Utilities/Material/Material.h` | Material de superficie PBR |
| `Light` (subclases) | `Utilities/Light/Light.h` | Contribuciones de iluminación |
| `PhysicsBody` | `Utilities/Physics/PhysicsBody.h` | Física de cuerpo rígido |
| `SkeletalComponent` | `Utilities/Animation/SkeletalComponent.h` | Jerarquía de esqueleto |
| `QEAnimationComponent` | `Utilities/Animation/QEAnimationComponent.h` | Reproducción de animaciones |
| `ParticleSystem` | `Utilities/Particles/ParticleSystem.h` | Emisor de partículas GPU |
| `ComputeNode` | `Utilities/Compute/ComputeNode.h` | Cómputo GPU de propósito general |
| `AtmosphereSystem` | `Utilities/Atmosphere/AtmosphereSystem.h` | Renderizado de cielo |
| `QECharacterController` | `Utilities/Controller/QECharacterController.h` | Movimiento de personaje |

---

## QEScene — el Contenedor de Escena

`QEScene` posee y gestiona todas las instancias de `QEGameObject`.

```cpp
class QEScene {
public:
    std::string name;
    std::vector<QEGameObject*> rootObjects;  // entidades de nivel raíz

    void AddGameObject(QEGameObject* obj);
    void RemoveGameObject(QEGameObject* obj);

    void Update(float deltaTime);
    void Render();

    bool Save(const std::string& filepath);
    bool Load(const std::string& filepath);
};
```

### Ciclo de Vida de la Escena

```
QEScene::Load(ruta)          // deserializar escena desde YAML
    └─ para cada objeto → crear QEGameObject + componentes

App::Run() por frame:
    QEScene::Update(dt)      // actualizar cada componente
    QEScene::Render()        // emitir draw calls

QEScene::Save(ruta)          // serializar estado actual a YAML
```

---

## GameObjectManager

`GameObjectManager` es un singleton que mantiene un registro global de todas las instancias vivas de `QEGameObject` (a través de todas las escenas):

```cpp
// Recuperar un game object por nombre
QEGameObject* jugador = GameObjectManager::Instance()->Find("Player");

// Iterar todos los objetos
for (auto* obj : GameObjectManager::Instance()->GetAll()) { ... }
```

---

## IDs Únicos — `Numbered`

Cada `QEGameObject` y `QEGameComponent` hereda de `Numbered`, que asigna automáticamente un ID entero monótonamente creciente en la construcción.  
Los IDs se usan internamente para búsquedas rápidas y referencias de serialización.

---

## Session Manager

`QESessionManager` rastrea la sesión del motor (apertura/cierre de proyectos, transiciones de escenas) y es el propietario raíz del ciclo de vida de `QEScene`.

---

## Crear un Game Object (ejemplo)

```cpp
// En el código de configuración de la escena
QEGameObject* caja = new QEGameObject("Caja");

// Añadir una malla
auto* geo = caja->AddComponent<QEGeometryComponent>();
geo->SetMesh(MeshImporter::Load("resources/models/box.glb"));

// Añadir un material
auto* mat = caja->AddComponent<Material>();
mat->SetAlbedo(glm::vec3(0.8f, 0.2f, 0.2f));

// Añadir física de cuerpo rígido
auto* body = caja->AddComponent<PhysicsBody>();
body->AddCollider<BoxCollider>(glm::vec3(0.5f)); // semiejes

// Registrar en la escena
scene->AddGameObject(caja);
```

---

## Ver también

- [Descripción de la Arquitectura](Arquitectura.md)
- [Serialización](Serializacion.md)
- [Sistema de Física](Sistema-Fisica.md)
- [Sistema de Animación](Sistema-Animacion.md)
