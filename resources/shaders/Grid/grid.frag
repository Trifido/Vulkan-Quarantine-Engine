#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_OES_standard_derivatives : enable

// Extensión del fade del grid
float gridSize = 140.0;

// Tamaño de celda base
float gridCellSize = 1.0;

// Color líneas finas
vec4 gridColorThin = vec4(0.62, 0.62, 0.62, 1.0);

// Color líneas grandes
vec4 gridColorThick = vec4(1.00, 0.60, 0.18, 1.0);

// Mínimo de píxeles entre líneas antes de cambiar de LOD
const float gridMinPixelsBetweenCells = 2.0;

float log10f(float x)
{
    return log(x) / log(10.0);
}

float satf(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x)
{
    return clamp(x, vec2(0.0), vec2(1.0));
}

float max2(vec2 v)
{
    return max(v.x, v.y);
}

vec4 gridColor(vec2 uv, vec2 camPos)
{
    vec2 dudv = vec2(
        length(vec2(dFdx(uv.x), dFdy(uv.x))),
        length(vec2(dFdx(uv.y), dFdy(uv.y)))
    );

    dudv = max(dudv, vec2(1e-6));

    float lodLevel = max(0.0, log10f((length(dudv) * gridMinPixelsBetweenCells) / gridCellSize) + 1.0);
    float lodFade = fract(lodLevel);

    float lod0 = gridCellSize * pow(10.0, floor(lodLevel));
    float lod1 = lod0 * 10.0;
    float lod2 = lod1 * 10.0;

    // cada línea AA puede ocupar hasta 4 píxeles
    dudv *= 4.0;

    // centrar líneas AA
    uv += dudv * 0.5;

    float lod0a = max2(vec2(1.0) - abs(satv(mod(uv, lod0) / dudv) * 2.0 - vec2(1.0)));
    float lod1a = max2(vec2(1.0) - abs(satv(mod(uv, lod1) / dudv) * 2.0 - vec2(1.0)));
    float lod2a = max2(vec2(1.0) - abs(satv(mod(uv, lod2) / dudv) * 2.0 - vec2(1.0)));

    vec2 localUV = uv - camPos;

    vec4 c = lod2a > 0.0
        ? gridColorThick
        : lod1a > 0.0
            ? mix(gridColorThick, gridColorThin, lodFade)
            : gridColorThin;

    // Fade radial suave alrededor de cámara
    float opacityFalloff = 1.0 - satf(length(localUV) / gridSize);

    // reforzar un poco el centro y hacer más suave la caída
    opacityFalloff = smoothstep(0.0, 1.0, opacityFalloff);

    c.a *= (lod2a > 0.0 ? lod2a : lod1a > 0.0 ? lod1a : (lod0a * (1.0 - lodFade))) * opacityFalloff;

    return c;
}

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 camPos;
layout(location = 0) out vec4 out_FragColor;

void main()
{
    out_FragColor = gridColor(uv, camPos);
    if (out_FragColor.a <= 0.001)
    discard;
}