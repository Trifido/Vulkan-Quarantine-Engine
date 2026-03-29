# Animation System

> **Language:** English · [Español](../es/Sistema-Animacion.md)  
> ← [Home](Home.md)

---

## Overview

Quarantine Engine supports **skeletal animation** with **GPU-accelerated skinning** via compute shaders.  
The animation pipeline lives in `src/Utilities/Animation/` and uses **Assimp** for FBX / GLTF asset importing.

![Animation Screenshot](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c)

---

## Core Classes

| Class | File | Role |
|---|---|---|
| `SkeletalComponent` | `SkeletalComponent.h` | Component — owns the skeleton hierarchy and bind-pose data |
| `Bone` | `Bone.h` | A single bone with a name, parent index, and offset matrix |
| `Animation` | `Animation.h` | An animation clip (name, duration, frame data per bone) |
| `Animator` | `Animator.h` | Playback controller — interpolates between keyframes |
| `QEAnimationComponent` | `QEAnimationComponent.h` | ECS component combining `SkeletalComponent` + `Animator` |
| `AnimationImporter` | `AnimationImporter.h` | Loads `.fbx` / `.gltf` animations via Assimp |
| `AnimationResources` | `AnimationResources.h` | GPU SSBOs for bone matrices |
| `QEAnimationResources` | `QEAnimationResources.h` | Per-frame resource management |
| `QEAnimationStateData` | `QEAnimationStateData.h` | Serialisable animation state (clip, speed, loop) |

---

## Skeletal Hierarchy

A skeleton is a tree of `Bone` nodes.  
Each bone stores:
- **Name** — matched to mesh joint names from the FBX/GLTF file.
- **Parent index** — for matrix chain propagation.
- **Offset matrix** — transforms from mesh space into bone local space (inverse bind pose).

---

## Loading a Skinned Mesh

```cpp
// AnimationImporter handles both mesh and skeleton import
auto* animComp = obj->AddComponent<QEAnimationComponent>();
animComp->Load("resources/models/Artorias/Artorias.fbx");

// The component now owns:
//   - SkeletalComponent (bone hierarchy + bind pose)
//   - Animation clips loaded from the file
```

---

## Playing Animations

```cpp
QEAnimationComponent* anim = obj->GetComponent<QEAnimationComponent>();

// Play a named clip
anim->Play("Idle");
anim->Play("Run", /*loop=*/true, /*speed=*/1.5f);

// Stop / pause
anim->Stop();
anim->SetPaused(true);

// Blend (future)
anim->CrossFade("Walk", /*duration=*/0.3f);
```

---

## GPU Skinning Pipeline

CPU-side skinning is **not used**.  
Each frame the `Animator` computes the final bone-matrix palette (one `mat4` per bone) and uploads it to a GPU SSBO.  
A **compute shader dispatch** then transforms every vertex:

```
Per-frame (compute queue):
  Animator::Update(dt)
    → interpolate keyframes
    → propagate bone chain
    → upload bonePalette[N] to SSBO

  vkCmdDispatch(computeSkinning)
    → reads:  restPoseVertices SSBO + bonePalette SSBO + blendWeights SSBO
    → writes: skinnedVertices SSBO

Graphics pass:
  Reads skinnedVertices SSBO as vertex input
  → renders normally with default.vert / default.frag
```

**Shader:** `resources/shaders/Animation/computeSkinning.comp`

```glsl
layout(set=0, binding=0) readonly  buffer RestPose    { Vertex inVertices[]; };
layout(set=0, binding=1) readonly  buffer BoneWeights { BlendWeight weights[]; };
layout(set=0, binding=2) readonly  buffer BonePalette { mat4 bones[]; };
layout(set=0, binding=3) writeonly buffer Skinned     { Vertex outVertices[]; };

void main() {
    uint i = gl_GlobalInvocationID.x;
    vec4 pos = vec4(0);
    for (int b = 0; b < 4; b++) {
        pos += weights[i].weight[b] * (bones[weights[i].boneId[b]] * vec4(inVertices[i].position, 1.0));
    }
    outVertices[i].position = pos.xyz;
    // ... transform normals similarly
}
```

---

## Animation Clip Format

Animation data is stored in memory as:

```
Animation
  ├─ name: string
  ├─ duration: float   (seconds)
  ├─ ticksPerSecond: float
  └─ channels: map<boneName, KeyframeChannel>
       └─ KeyframeChannel
            ├─ positionKeys: [ (time, vec3) ]
            ├─ rotationKeys: [ (time, quat) ]
            └─ scaleKeys:    [ (time, vec3) ]
```

Interpolation is **linear** (LERP for position/scale, SLERP for rotation).

---

## Serialisation

Animation state is saved via `QEAnimationStateData`:

```yaml
animation:
  clip: Run
  speed: 1.0
  loop: true
  currentTime: 0.0
```

---

## See Also

- [ECS System](ECS-System.md)
- [Shader System](Shader-System.md)
- [Serialisation](Serialization.md)
