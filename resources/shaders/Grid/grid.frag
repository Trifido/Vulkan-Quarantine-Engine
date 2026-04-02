#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_OES_standard_derivatives : enable

#include "../Includes/QECommon.glsl"

layout(set = 0, binding = 0, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(location = 0) in vec2 v_ndc;
layout(location = 0) out vec4 out_FragColor;

// ============================================================
// CONFIG
// ============================================================

const float GRID_PLANE_Y = 0.0;

// Más espaciado, pero visible
const float GRID_CELL_THIN  = 5.0;
const float GRID_CELL_THICK = 10.0;
const float GRID_CELL_SUPER = 20.0;

// Fade
const float FADE_START = 1800.0;
const float FADE_END   = 4200.0;

// Colores
const vec4 COLOR_THIN  = vec4(0.60, 0.60, 0.60, 0.22);
const vec4 COLOR_THICK = vec4(0.78, 0.78, 0.78, 0.42);
const vec4 COLOR_SUPER = vec4(1.00, 0.85, 0.35, 0.85);

// Ejes
const vec4 COLOR_AXIS_X = vec4(1.00, 0.55, 0.15, 1.00);
const vec4 COLOR_AXIS_Z = vec4(1.00, 0.55, 0.15, 1.00);

// ============================================================

float saturate1(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec3 ReconstructWorldPos(vec2 ndc, float z)
{
    vec4 clip = vec4(ndc, z, 1.0);
    vec4 world = cameraData.invViewProj * clip;
    return world.xyz / world.w;
}

float GridLine(vec2 uv, float cellSize)
{
    vec2 coord = uv / cellSize;

    vec2 fw = vec2(
        length(vec2(dFdx(coord.x), dFdy(coord.x))),
        length(vec2(dFdx(coord.y), dFdy(coord.y)))
    );

    fw = max(fw, vec2(1e-4));

    vec2 g = abs(fract(coord - 0.5) - 0.5) / fw;
    float line = min(g.x, g.y);

    return 1.0 - saturate1(line);
}

float AxisLine(float coord, float widthWorld)
{
    float fw = max(fwidth(coord), 1e-6);
    return 1.0 - smoothstep(widthWorld - fw, widthWorld + fw, abs(coord));
}

void main()
{
    vec3 worldNear = ReconstructWorldPos(v_ndc, -1.0);
    vec3 worldFar  = ReconstructWorldPos(v_ndc,  1.0);

    vec3 rayOrigin = worldNear;
    vec3 rayDir = normalize(worldFar - worldNear);

    float denom = rayDir.y;
    if (abs(denom) < 1e-6)
        discard;

    float t = (GRID_PLANE_Y - rayOrigin.y) / denom;
    if (t <= 0.0)
        discard;

    vec3 hit = rayOrigin + rayDir * t;

    vec4 clip = cameraData.viewProjection * vec4(hit, 1.0);

    float depth = clip.z / clip.w;
    depth = depth * 0.5 + 0.5;

    if (depth < 0.0 || depth > 1.0)
        discard;

    gl_FragDepth = depth;

    vec2 uv = hit.xz;

    float distToCamera = distance(cameraData.position.xyz, hit);
    float fade = 1.0 - smoothstep(FADE_START, FADE_END, distToCamera);

    float horizonFade = abs(rayDir.y);
    horizonFade = smoothstep(0.02, 0.25, horizonFade);

    float thinLine  = GridLine(uv, GRID_CELL_THIN);
    float thickLine = GridLine(uv, GRID_CELL_THICK);
    float superLine = GridLine(uv, GRID_CELL_SUPER);

    // Jerarquía real:
    // SUPER manda sobre THICK, THICK manda sobre THIN
    float superMask = superLine;
    float thickMask = thickLine * (1.0 - superMask);
    float thinMask  = thinLine  * (1.0 - thickLine);

    float axisWidth = 0.09;
    float axisX = AxisLine(hit.z, axisWidth);
    float axisZ = AxisLine(hit.x, axisWidth);

    vec4 color = vec4(0.0);

    // Base gris
    color = mix(color, COLOR_THIN,  thinMask  * COLOR_THIN.a);

    // Líneas grandes
    color = mix(color, COLOR_THICK, thickMask * COLOR_THICK.a);

    // Superceldas amarillas
    color = mix(color, COLOR_SUPER, superMask * COLOR_SUPER.a);

    // Ejes por encima de todo
    color.rgb = mix(color.rgb, COLOR_AXIS_X.rgb, axisX * COLOR_AXIS_X.a);
    color.a   = max(color.a, axisX * COLOR_AXIS_X.a);

    color.rgb = mix(color.rgb, COLOR_AXIS_Z.rgb, axisZ * COLOR_AXIS_Z.a);
    color.a   = max(color.a, axisZ * COLOR_AXIS_Z.a);

    color.a *= fade;
    color.a *= horizonFade;

    if (color.a <= 0.001)
        discard;

    out_FragColor = color;
}