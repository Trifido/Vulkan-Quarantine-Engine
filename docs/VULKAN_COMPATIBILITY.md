# Perfil de compatibilidad Vulkan

Este documento define las capacidades que Quarantine Engine negocia durante el arranque. El objetivo es que una GPU incompatible se descarte con un diagnóstico útil y que ninguna feature llegue a `vkCreateDevice` sin haber sido consultada antes.

## Perfil estable requerido

El editor y el runtime requieren:

- Una GPU y un driver compatibles con Vulkan 1.3.
- Colas de gráficos, compute y presentación.
- La extensión de dispositivo `VK_KHR_swapchain` y una superficie con formatos y modos de presentación disponibles.
- `shaderSampledImageArrayNonUniformIndexing`, usado por los arrays de texturas y mapas de sombras.
- `extendedDynamicState`, usado para cambiar culling, front face y estado de profundidad al grabar command buffers.

Durante la selección se evalúan todas las GPUs. Las incompatibles se registran junto con el motivo del rechazo y, entre las compatibles, se prioriza una GPU dedicada sobre una integrada.

## Capacidades opcionales

Las siguientes features sólo se activan cuando la GPU confirma su soporte:

| Capacidad | Comportamiento sin soporte |
|---|---|
| `sampleRateShading` | Los pipelines desactivan sample shading. |
| `fillModeNonSolid` | Los pipelines `Line` o `Point` se rechazan con un error explícito; `Fill` continúa funcionando. |
| `wideLines` | Un pipeline con ancho distinto de `1.0` se rechaza con un error explícito. |
| `bufferDeviceAddress` | El render convencional funciona; los buffers que soliciten dirección de dispositivo se rechazan. |
| `VK_EXT_mesh_shader` / mesh shader | El pipeline raster convencional funciona; un asset de mesh shader se rechaza con un error explícito. |
| task shader | Sólo se habilita si mesh shader y task shader están soportados; los assets sin etapa task pueden seguir usando mesh shader. |

Los buffers de geometría del perfil estable no solicitan `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`. Esa capacidad queda reservada para rutas experimentales que realmente consumen direcciones GPU.

## Fuera del perfil estable

- Ray tracing sigue siendo experimental y no negocia todavía el conjunto completo de extensiones y features que necesita.
- Fragment shading rate y pipeline libraries no se habilitan porque el motor estable no las consume actualmente.
- Mesh shaders son opcionales por asset. Un proyecto que los use debe conservar una alternativa raster si necesita ejecutarse en GPUs sin `VK_EXT_mesh_shader`.

## Diagnóstico esperado

En cada arranque el log Vulkan identifica:

- GPUs compatibles y rechazadas.
- GPU seleccionada, versión de API, vendor ID, device ID y versión de driver.
- Estado de buffer device address, indexación no uniforme, dynamic state, mesh/task shaders, sample shading, wireframe y líneas anchas.

Para validar un cambio de compatibilidad deben probarse al menos el arranque, la escena por defecto, resize/minimize/restore y el cierre con validation layers activas en Debug.
