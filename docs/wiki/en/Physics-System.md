# Physics System

> **Language:** English · [Español](../es/Sistema-Fisica.md)  
> ← [Home](Home.md)

---

## Overview

Quarantine Engine integrates **Jolt Physics** (`extern/jolt`) for rigid-body simulation.  
The wrapper lives in `src/Utilities/Physics/` and exposes a component-based API consistent with the rest of the ECS.

![Physics Screenshot](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/49e7249e-d57e-4693-9d61-8ca243906290)

---

## Core Classes

| Class | File | Role |
|---|---|---|
| `PhysicsModule` | `PhysicsModule.h` | Singleton — owns the Jolt `PhysicsSystem`, step scheduler, and debug renderer |
| `PhysicsBody` | `PhysicsBody.h` | Component — attaches a Jolt body to a game object |
| `Collider` | `Collider.h` | Abstract base for all collision shapes |
| `BoxCollider` | `BoxCollider.h` | Axis-aligned box shape (AABB half-extents) |
| `SphereCollider` | `SphereCollider.h` | Sphere shape (radius) |
| `CapsuleCollider` | `CapsuleCollider.h` | Capsule shape (radius + half-height) |
| `PlaneCollider` | `PlaneCollider.h` | Infinite plane (static only) |
| `AABBObject` | `AABBObject.h` | Utility AABB for broadphase queries |
| `JoltDebugRenderer` | `JoltDebugRenderer.h` | Renders Jolt debug shapes via the engine debug draw system |
| `QECharacterController` | `Controller/QECharacterController.h` | Kinematic character controller (Jolt `CharacterVirtual`) |

---

## Initialisation

`PhysicsModule::Instance()` initialises Jolt Physics once on first access:

```cpp
// Engine startup (handled by App::Init)
PhysicsModule::Instance()->Init(
    maxBodies       = 1024,
    maxBodyPairs    = 65536,
    maxContactConstraints = 10240
);
```

---

## Adding Physics to a Game Object

### Static Body (floor, wall)

```cpp
auto* floor = new QEGameObject("Floor");
auto* body  = floor->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Static);
body->AddCollider<PlaneCollider>();      // infinite ground plane
```

### Dynamic Body

```cpp
auto* box = new QEGameObject("Box");
auto* body = box->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Dynamic);
body->AddCollider<BoxCollider>(glm::vec3(0.5f));  // half-extents
body->SetMass(5.0f);
body->SetLinearVelocity(glm::vec3(0.0f, 10.0f, 0.0f));
```

### Sphere

```cpp
auto* ball = new QEGameObject("Ball");
auto* body = ball->AddComponent<PhysicsBody>();
body->SetMotionType(EMotionType::Dynamic);
body->AddCollider<SphereCollider>(0.5f);  // radius
```

### Capsule

```cpp
auto* body = obj->AddComponent<PhysicsBody>();
body->AddCollider<CapsuleCollider>(0.4f, 0.9f);  // radius, halfHeight
```

---

## Per-Frame Physics Update

```cpp
// Called once per frame in App::Run()
PhysicsModule::Instance()->Update(deltaTime);
```

After the physics step, all `PhysicsBody` components copy the Jolt body transform back to `QETransform`, keeping the visual representation in sync.

---

## Character Controller

`QECharacterController` wraps Jolt's `CharacterVirtual` for player-controlled characters:

```cpp
auto* cc = playerObj->AddComponent<QECharacterController>();
cc->SetMaxSlopeAngle(45.0f);       // degrees
cc->SetMaxStrength(100.0f);

// Move every frame (call from input handler)
cc->SetLinearVelocity(inputDir * speed);
cc->Jump(5.0f);                    // vertical impulse
```

---

## Debug Visualisation

Enable Jolt debug rendering via `JoltDebugRenderer`:

```cpp
PhysicsModule::Instance()->SetDebugDraw(true);
```

This renders all collision shapes as wireframe overlays using the engine's debug shader (`debugAABB.vert/frag`).

---

## Collision Filtering (Layers)

Jolt's layer system is mapped to named engine layers:

| Layer | Index | Typical Use |
|---|---|---|
| `NON_MOVING` | 0 | Static world geometry |
| `MOVING` | 1 | Dynamic and kinematic objects |
| `TRIGGERS` | 2 | Overlap volumes (no collision response) |

---

## See Also

- [ECS System](ECS-System.md)
- [Architecture Overview](Architecture.md)
