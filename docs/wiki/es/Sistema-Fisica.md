# Sistema de Física

> **Idioma:** [English](../en/Physics-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Quarantine Engine integra **Jolt Physics** (`extern/jolt`) para simulación de cuerpos rígidos.  
El wrapper reside en `src/Utilities/Physics/` y expone una API basada en componentes consistente con el resto del ECS.

![Captura del Sistema de Física](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290)

---

## Clases Principales

| Clase | Archivo | Rol |
|---|---|---|
| `PhysicsModule` | `PhysicsModule.h` | Singleton — posee el `PhysicsSystem` de Jolt, el planificador de pasos y el renderer de depuración |
| `PhysicsBody` | `PhysicsBody.h` | Componente — adjunta un cuerpo Jolt a un game object |
| `Collider` | `Collider.h` | Base abstracta para todas las formas de colisión |
| `BoxCollider` | `BoxCollider.h` | Forma de caja alineada con los ejes (semiejes AABB) |
| `SphereCollider` | `SphereCollider.h` | Forma esférica (radio) |
| `CapsuleCollider` | `CapsuleCollider.h` | Forma de cápsula (radio + semi-altura) |
| `PlaneCollider` | `PlaneCollider.h` | Plano infinito (solo estático) |
| `AABBObject` | `AABBObject.h` | AABB utilitario para consultas de broadphase |
| `JoltDebugRenderer` | `JoltDebugRenderer.h` | Renderiza formas de depuración de Jolt vía el sistema de depuración del motor |
| `QECharacterController` | `Controller/QECharacterController.h` | Controlador de personaje cinemático (Jolt `CharacterVirtual`) |

---

## Inicialización

`PhysicsModule::Instance()` inicializa Jolt Physics una vez en el primer acceso:

```cpp
// Inicio del motor (gestionado por App::Init)
PhysicsModule::Instance()->Init(
    maxBodies       = 1024,
    maxBodyPairs    = 65536,
    maxContactConstraints = 10240
);
```

---

## Añadir Física a un Game Object

### Cuerpo Estático (suelo, pared)

```cpp
auto* suelo = new QEGameObject("Suelo");
auto* body  = suelo->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Static);
body->AddCollider<PlaneCollider>();      // plano de suelo infinito
```

### Cuerpo Dinámico

```cpp
auto* caja = new QEGameObject("Caja");
auto* body = caja->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Dynamic);
body->AddCollider<BoxCollider>(glm::vec3(0.5f));  // semiejes
body->SetMass(5.0f);
body->SetLinearVelocity(glm::vec3(0.0f, 10.0f, 0.0f));
```

### Esfera

```cpp
auto* bola = new QEGameObject("Bola");
auto* body = bola->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Dynamic);
body->AddCollider<SphereCollider>(0.5f);  // radio
```

### Cápsula

```cpp
auto* body = obj->AddComponent<PhysicsBody>();
body->AddCollider<CapsuleCollider>(0.4f, 0.9f);  // radio, semiAltura
```

---

## Actualización de Física por Frame

```cpp
// Llamado una vez por frame en App::Run()
PhysicsModule::Instance()->Update(deltaTime);
```

Después del paso de física, todos los componentes `PhysicsBody` copian la transformación del cuerpo Jolt de vuelta a `QETransform`, manteniendo la representación visual sincronizada.

---

## Controlador de Personaje

`QECharacterController` envuelve el `CharacterVirtual` de Jolt para personajes controlados por el jugador:

```cpp
auto* cc = playerObj->AddComponent<QECharacterController>();
cc->SetMaxSlopeAngle(45.0f);       // grados
cc->SetMaxStrength(100.0f);

// Mover cada frame (llamar desde el gestor de entrada)
cc->SetLinearVelocity(inputDir * speed);
cc->Jump(5.0f);                    // impulso vertical
```

---

## Visualización de Depuración

Habilita el renderizado de depuración de Jolt mediante `JoltDebugRenderer`:

```cpp
PhysicsModule::Instance()->SetDebugDraw(true);
```

Esto renderiza todas las formas de colisión como overlays en wireframe usando el shader de depuración del motor (`debugAABB.vert/frag`).

---

## Filtrado de Colisiones (Capas)

El sistema de capas de Jolt se mapea a capas del motor con nombre:

| Capa | Índice | Uso típico |
|---|---|---|
| `NON_MOVING` | 0 | Geometría estática del mundo |
| `MOVING` | 1 | Objetos dinámicos y cinemáticos |
| `TRIGGERS` | 2 | Volúmenes de overlap (sin respuesta de colisión) |

---

## Ver también

- [Sistema ECS](Sistema-ECS.md)
- [Descripción de la Arquitectura](Arquitectura.md)
