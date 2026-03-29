# Sistema de Animación

> **Idioma:** [English](../en/Animation-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

Quarantine Engine soporta **animación esquelética** con **skinning acelerado por GPU** vía compute shaders.  
El pipeline de animación reside en `src/Utilities/Animation/` y usa **Assimp** para importar activos FBX / GLTF.

![Captura del Sistema de Animación](https://github.com/Trifido/Vulkan-Quarantine-Engine/assets/6890573/08d68027-12a0-4d75-a26d-67d48cfe353c)

---

## Clases Principales

| Clase | Archivo | Rol |
|---|---|---|
| `SkeletalComponent` | `SkeletalComponent.h` | Componente — posee la jerarquía de esqueleto y datos de pose en reposo |
| `Bone` | `Bone.h` | Un hueso único con nombre, índice de padre y matriz de offset |
| `Animation` | `Animation.h` | Un clip de animación (nombre, duración, datos de frame por hueso) |
| `Animator` | `Animator.h` | Controlador de reproducción — interpola entre keyframes |
| `QEAnimationComponent` | `QEAnimationComponent.h` | Componente ECS que combina `SkeletalComponent` + `Animator` |
| `AnimationImporter` | `AnimationImporter.h` | Carga animaciones `.fbx` / `.gltf` vía Assimp |
| `AnimationResources` | `AnimationResources.h` | SSBOs GPU para matrices de huesos |
| `QEAnimationResources` | `QEAnimationResources.h` | Gestión de recursos por frame |
| `QEAnimationStateData` | `QEAnimationStateData.h` | Estado de animación serializable (clip, velocidad, bucle) |

---

## Jerarquía Esquelética

Un esqueleto es un árbol de nodos `Bone`.  
Cada hueso almacena:
- **Nombre** — coincide con los nombres de joint de la malla del archivo FBX/GLTF.
- **Índice de padre** — para la propagación de la cadena de matrices.
- **Matriz de offset** — transforma del espacio de malla al espacio local del hueso (pose inversa de bind).

---

## Cargar una Malla con Skinning

```cpp
// AnimationImporter gestiona tanto la importación de malla como del esqueleto
auto* animComp = obj->AddComponent<QEAnimationComponent>();
animComp->Load("resources/models/Artorias/Artorias.fbx");

// El componente ahora posee:
//   - SkeletalComponent (jerarquía de huesos + pose de bind)
//   - Clips de animación cargados desde el archivo
```

---

## Reproducir Animaciones

```cpp
QEAnimationComponent* anim = obj->GetComponent<QEAnimationComponent>();

// Reproducir un clip por nombre
anim->Play("Idle");
anim->Play("Run", /*loop=*/true, /*speed=*/1.5f);

// Detener / pausar
anim->Stop();
anim->SetPaused(true);

// Mezcla (futuro)
anim->CrossFade("Walk", /*duration=*/0.3f);
```

---

## Pipeline de Skinning GPU

El skinning en CPU **no se usa**.  
Cada frame el `Animator` calcula la paleta final de matrices de huesos (una `mat4` por hueso) y la sube a un SSBO GPU.  
Un **dispatch de compute shader** transforma entonces cada vértice:

```
Por frame (cola de cómputo):
  Animator::Update(dt)
    → interpolar keyframes
    → propagar cadena de huesos
    → subir bonePalette[N] al SSBO

  vkCmdDispatch(computeSkinning)
    → lee:  SSBO de vértices en pose en reposo + SSBO bonePalette + SSBO blendWeights
    → escribe: SSBO de vértices con skinning

Pasada de gráficos:
  Lee SSBO de vértices con skinning como entrada de vértices
  → renderiza normalmente con default.vert / default.frag
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
    // ... transformar normales de manera similar
}
```

---

## Formato de Clip de Animación

Los datos de animación se almacenan en memoria como:

```
Animation
  ├─ name: string
  ├─ duration: float   (segundos)
  ├─ ticksPerSecond: float
  └─ channels: map<boneName, KeyframeChannel>
       └─ KeyframeChannel
            ├─ positionKeys: [ (tiempo, vec3) ]
            ├─ rotationKeys: [ (tiempo, quat) ]
            └─ scaleKeys:    [ (tiempo, vec3) ]
```

La interpolación es **lineal** (LERP para posición/escala, SLERP para rotación).

---

## Serialización

El estado de animación se guarda mediante `QEAnimationStateData`:

```yaml
animation:
  clip: Run
  speed: 1.0
  loop: true
  currentTime: 0.0
```

---

## Ver también

- [Sistema ECS](Sistema-ECS.md)
- [Sistema de Shaders](Sistema-Shaders.md)
- [Serialización](Serializacion.md)
