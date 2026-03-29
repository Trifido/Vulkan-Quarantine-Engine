# Sistema de Materiales

> **Idioma:** [English](../en/Material-System.md) · Español  
> ← [Inicio](Inicio.md)

---

## Descripción General

El sistema de materiales (`src/Utilities/Material/`) gestiona las propiedades de superficie PBR, los slots de texturas y la asignación de recursos GPU para toda la geometría renderizada.

---

## Material

`Material` es un `QEGameComponent` que almacena parámetros PBR y un conjunto de slots de texturas.

### Propiedades PBR

| Propiedad | Tipo | Por defecto | Descripción |
|---|---|---|---|
| `albedo` | `glm::vec3` | `(1,1,1)` | Color base (lineal) |
| `metallic` | `float` | `0.0` | 0 = dieléctrico, 1 = metálico |
| `roughness` | `float` | `0.5` | Rugosidad de la microsuperficie |
| `ambientOcclusion` | `float` | `1.0` | Multiplicador de oclusión ambiental |
| `emissive` | `glm::vec3` | `(0,0,0)` | Color de auto-emisión |
| `emissiveIntensity` | `float` | `0.0` | Multiplicador de brillo de emisión |
| `opacity` | `float` | `1.0` | Alpha (1 = opaco) |

### Slots de Texturas

| Slot | Enum | Mapa típico |
|---|---|---|
| Albedo / Color Base | `ALBEDO` | Mapa de color difuso |
| Normal | `NORMAL` | Mapa normal en espacio tangente |
| Metallic-Roughness | `METALLIC_ROUGHNESS` | Textura MR combinada (ORM) |
| Oclusión Ambiental | `AO` | Mapa AO horneado |
| Emisivo | `EMISSIVE` | Mapa de auto-iluminación |
| Altura / Desplazamiento | `HEIGHT` | Parallax / teselación |

Definiciones de slots: `src/Utilities/Material/MaterialTextureSlot.h`  
Enum de slots: `src/Utilities/Material/TextureTypes.h`

---

## Usar Materiales

```cpp
// Crear y configurar un material
auto* mat = gameObject->AddComponent<Material>();
mat->albedo    = glm::vec3(0.9f, 0.85f, 0.8f);
mat->metallic  = 0.0f;
mat->roughness = 0.4f;

// Asignar texturas
auto* texMgr = TextureManager::Instance();
mat->SetTexture(ALBEDO,  texMgr->Load("resources/textures/wall/brickwall.jpg"));
mat->SetTexture(NORMAL,  texMgr->Load("resources/textures/wall/brickwall_normal.jpg"));
```

---

## MaterialManager

`MaterialManager` (singleton) mantiene un registro de todos los materiales cargados y proporciona búsqueda por nombre.

```cpp
Material* mat = MaterialManager::Instance()->Get("BrickWall");
```

Los materiales se pueden serializar y deserializar mediante `MaterialDto` (consulta [Serialización](Serializacion.md)).

---

## TextureManager

`TextureManager` (singleton) cachea los objetos `CustomTexture` cargados.  
Las texturas se suben a objetos `VkImage` residentes en GPU mediante `TextureManagerModule` (`src/Memory/TextureManagerModule.h`).

```cpp
CustomTexture* tex = TextureManager::Instance()->Load("resources/textures/container2.png");
```

Formatos soportados:
- **LDR:** PNG, JPG, BMP (vía `stb_image`)
- **HDR:** `.hdr` Radiance RGBE (para mapas de entorno/skybox)

---

## ShaderManager

`ShaderManager` (singleton) compila shaders GLSL a SPIR-V en el primer uso y cachea los objetos `VkShaderModule` resultantes.

```cpp
ShaderModule* vs = ShaderManager::Instance()->Get("resources/shaders/Default/default.vert.spv");
```

Los archivos `.spv` precompilados se incluyen en el repositorio; el manager los cargará directamente si están presentes.

---

## CustomTexture

`CustomTexture` encapsula una tripla `VkImage` + `VkImageView` + `VkSampler`.  
Rastrea el formato de imagen, el número de niveles mip y el número de capas (1 para 2D, 6 para cubemaps).

---

## Serialización de Materiales

Los materiales se serializan a YAML mediante `MaterialDto`:

```yaml
material:
  name: BrickWall
  albedo: [0.9, 0.85, 0.8]
  metallic: 0.0
  roughness: 0.4
  textures:
    albedo: resources/textures/wall/brickwall.jpg
    normal: resources/textures/wall/brickwall_normal.jpg
```

---

## Ver también

- [Sistema de Shaders](Sistema-Shaders.md)
- [Pipeline de Renderizado](Pipeline-Renderizado.md)
- [Serialización](Serializacion.md)
