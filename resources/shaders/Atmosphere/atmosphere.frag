#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../Includes/QECommon.glsl"

layout(binding = 0) uniform sampler2D InputImage;
layout(binding = 1) uniform sampler2D InputImage_2;

layout(set = 0, binding = 2, std140) uniform UniformCamera
{
    QECameraData cameraData;
};

layout(set = 0, binding = 3) uniform ScreenResolution
{
    vec2 data;
} screenResolution;

layout(set = 0, binding = 4) uniform SunUniform
{
    vec3 direction;
    float intensity;
} sunData;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358;

// Unidades en Megametros (MM)
const float PlanetRadius = 6.360;
const float AtmosphereRadius = 6.460;
const float AtmosphereExposure = 12.0;
const float HorizonBias = -0.0028;

float safeAcos(float x)
{
    return acos(clamp(x, -1.0, 1.0));
}

const vec2 tLUTRes = vec2(256.0, 64.0);
vec3 getValFromTLUT(vec3 viewPos, vec3 sunDir)
{
    float height = length(viewPos);
    vec3 up = viewPos / height;
    float sunCosZenithAngle = dot(sunDir, up);

    vec2 uv = vec2(
        clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0),
        clamp((height - PlanetRadius) / (AtmosphereRadius - PlanetRadius), 0.0, 1.0)
    );

    return texture(InputImage, uv).rgb;
}

const vec2 skyLUTRes = vec2(640.0, 360.0);
vec3 getValFromSkyLUT(vec3 viewPos, vec3 rayDir, vec3 sunDir)
{
    float height = length(viewPos);
    vec3 up = viewPos / height;

    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height);
    float altitudeAngle = horizonAngle - acos(clamp(dot(rayDir, up), -1.0, 1.0));

    float azimuthAngle;
    if (abs(altitudeAngle) > (0.5 * PI - 0.0001))
    {
        azimuthAngle = 0.0;
    }
    else
    {
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);

        vec3 projectedDir = normalize(rayDir - up * dot(rayDir, up));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);
        azimuthAngle = atan(sinTheta, cosTheta) + PI;
    }

    float v = 0.5 + 0.5 * sign(altitudeAngle) * sqrt(abs(altitudeAngle) * 2.0 / PI);
    vec2 uv = vec2(azimuthAngle / (2.0 * PI), v);

    return texture(InputImage_2, uv).rgb;
}

vec3 jodieReinhardTonemap(vec3 c)
{
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.0);
    return mix(c / (l + 1.0), tc, tc);
}

vec3 sunDisk(vec3 rayDir, vec3 sunDir)
{
    const float sunAngularRadius = 0.73 * PI / 180.0;
    float cosTheta = dot(rayDir, sunDir);
    float minSunCosTheta = cos(sunAngularRadius);

    float eps = 0.000015;
    return vec3(smoothstep(minSunCosTheta - eps, minSunCosTheta + eps, cosTheta));
}

vec3 sunBloom(vec3 rayDir, vec3 sunDir)
{
    float cosTheta = dot(rayDir, sunDir);
    float offset = max(0.0, 1.0 - cosTheta);

    float wideBloom = exp(-offset * 3500.0) * 0.22;
    float tightBloom = exp(-offset * 12000.0) * 0.08;
    float invBloom = 1.0 / (0.008 + offset * 180.0) * 0.0018;

    return vec3(wideBloom + tightBloom + invBloom);
}

vec3 safeNormalizeColor(vec3 c)
{
    float m = max(c.r, max(c.g, c.b));
    return (m > 1e-5) ? (c / m) : vec3(1.0);
}

void main()
{
    vec2 iResolution = screenResolution.data;

    vec3 viewPos = cameraData.position.xyz * 1e-6;
    viewPos.y += PlanetRadius;
    viewPos.y = max(PlanetRadius + 1e-6, viewPos.y);

    vec3 sunDirWorld = normalize(sunData.direction);

    float height = length(viewPos);
    vec3 up = viewPos / height;

    float sunAltitude = (0.5 * PI) - acos(clamp(dot(sunDirWorld, up), -1.0, 1.0));
    vec3 sunDir = normalize(vec3(0.0, sin(sunAltitude), -cos(sunAltitude)));

    vec2 ndc = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;

    vec4 clip = vec4(ndc, 1.0, 1.0);
    vec4 viewPosFar = cameraData.invProj * clip;
    viewPosFar /= viewPosFar.w;

    vec3 rayDirView = normalize(-viewPosFar.xyz);
    vec3 rayDir = normalize((cameraData.invView * vec4(rayDirView, 0.0)).xyz);

    // Cielo
    vec3 skyLum = getValFromSkyLUT(viewPos, rayDir, sunDir);
    skyLum *= 1.25;

    // Sol
    vec3 disk = sunDisk(rayDir, sunDir);
    vec3 bloom = sunBloom(rayDir, sunDir);

    // Recorte del sol con el mismo horizonte angular + bias
    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height) + HorizonBias;
    float viewZenithAngle = acos(clamp(dot(rayDir, up), -1.0, 1.0));
    float altitudeAngle = viewZenithAngle - horizonAngle;

    float diskVisible = smoothstep(-0.0005, 0.0005, altitudeAngle);
    float bloomVisible = smoothstep(-0.003, 0.003, altitudeAngle);

    disk *= diskVisible;
    bloom *= bloomVisible;

    // Color del sol
    vec3 sunTransmittance = getValFromTLUT(viewPos, sunDir);

    vec3 diskColor = safeNormalizeColor(sunTransmittance);
    float diskStrength = max(sunTransmittance.r, max(sunTransmittance.g, sunTransmittance.b));
    diskStrength = max(diskStrength, 0.25);

    float sunHeight = clamp(abs(sunDir.y), 0.0, 1.0);
    float dayAmount = smoothstep(0.02, 0.35, sunHeight);

    vec3 sunsetTint = vec3(1.0, 0.22, 0.04);
    vec3 dayTint    = vec3(1.0, 0.97, 0.92);

    vec3 sunDiskTint = mix(sunsetTint, dayTint, dayAmount);
    sunDiskTint *= mix(diskColor, vec3(1.0), dayAmount * 0.65);

    vec3 sunBloomTint = mix(vec3(1.0, 0.35, 0.08), vec3(1.0, 0.95, 0.9), dayAmount);

    disk *= sunDiskTint;
    disk *= mix(0.35, 1.0, diskStrength);

    vec3 bloomTransmittance = mix(vec3(1.0), sunTransmittance, 0.2);
    bloomTransmittance = max(bloomTransmittance, vec3(0.02));
    bloom *= bloomTransmittance * sunBloomTint;

    vec3 sky = jodieReinhardTonemap(skyLum * AtmosphereExposure);
    vec3 sun = disk * 3.5 + bloom * 5.0;

    vec3 finalColor = sky + sun;
    outColor = vec4(finalColor, 1.0);
}