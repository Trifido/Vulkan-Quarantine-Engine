#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../Includes/QECommon.glsl"
#include "../Includes/Atmosphere/QEAtmosphereCommon.glsl"
#include "../Includes/Atmosphere/QEAtmosphereFragCommon.glsl"

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

const vec2 tLUTRes = vec2(256.0, 64.0);
const vec2 skyLUTRes = vec2(640.0, 360.0);

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

vec3 getValFromSkyLUT(vec3 viewPos, vec3 localRayDir)
{
    float height = length(viewPos);

    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height) - 0.5 * PI;

    float altitudeAngle = asin(clamp(localRayDir.y, -1.0, 1.0));
    float azimuthAngle = atan(localRayDir.x, -localRayDir.z);

    float u = azimuthAngle / (2.0 * PI) + 0.5;

    float adjV = (altitudeAngle + horizonAngle) / (0.5 * PI);
    adjV = clamp(adjV, -1.0, 1.0);

    float v;
    if (adjV >= 0.0)
    {
        v = 0.5 * (1.0 - sqrt(adjV));
    }
    else
    {
        v = 0.5 * (1.0 + sqrt(-adjV));
    }

    return texture(InputImage_2, vec2(u, v)).rgb;
}

void main()
{
    vec2 iResolution = screenResolution.data;

    vec3 viewPos = cameraData.position.xyz * 1e-6;
    viewPos.y += PlanetRadius;
    viewPos.y = max(PlanetRadius + 1e-6, viewPos.y);

    vec2 ndc = (gl_FragCoord.xy / iResolution) * 2.0 - 1.0;

    vec4 clip = vec4(ndc, 1.0, 1.0);
    vec4 viewPosFar = cameraData.invProj * clip;
    viewPosFar /= viewPosFar.w;

    vec3 rayDirView = normalize(-viewPosFar.xyz);
    vec3 rayDir = normalize((cameraData.invView * vec4(rayDirView, 0.0)).xyz);

    vec3 sunDirWorld = normalize(sunData.direction);

    float height = length(viewPos);
    vec3 up = viewPos / height;

    vec3 refAxis = (abs(up.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, refAxis));
    vec3 forward = normalize(cross(right, up));

    vec3 localRayDir = normalize(vec3(
        dot(rayDir, right),
        -dot(rayDir, up),
        dot(rayDir, -forward)
    ));

    vec3 skyLum = getValFromSkyLUT(viewPos, localRayDir);
    vec3 disk = sunDisk(rayDir, sunDirWorld);
    vec3 bloom = sunBloom(rayDir, sunDirWorld);
    vec3 sunTransmittance = getValFromTLUT(viewPos, sunDirWorld);
    float sunHeight = clamp(sunDirWorld.y, 0.0, 1.0);

    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height) + HorizonBias;
    float viewZenithAngle = acos(clamp(dot(rayDir, up), -1.0, 1.0));
    float altitudeAngle = viewZenithAngle - horizonAngle;

    float diskVisible = smoothstep(-0.0005, 0.0005, altitudeAngle);
    float bloomVisible = smoothstep(-0.003, 0.003, altitudeAngle);

    disk *= diskVisible;
    bloom *= bloomVisible;

    vec3 diskColor = safeNormalizeColor(sunTransmittance);
    float diskStrength = max(sunTransmittance.r, max(sunTransmittance.g, sunTransmittance.b));
    diskStrength = max(diskStrength, 0.25);

    float dayAmount = smoothstep(0.02, 0.35, sunHeight);

    vec3 sunsetTint = vec3(1.0, 0.22, 0.04);
    vec3 dayTint = vec3(1.0, 0.97, 0.92);

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