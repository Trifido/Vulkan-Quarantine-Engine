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

    vec3 sunDirForDisk = normalize(sunData.direction);
    vec3 sunDirForTlut = normalize(-sunData.direction);

    vec3 disk = sunDisk(rayDir, sunDirForDisk);
    vec3 bloom = sunBloom(rayDir, sunDirForDisk);
    vec3 sunTransmittance = getValFromTLUT(viewPos, sunDirForTlut);

    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height) + HorizonBias;
    float viewZenithAngle = acos(clamp(dot(rayDir, up), -1.0, 1.0));
    float altitudeAngle = viewZenithAngle - horizonAngle;

    float diskVisible = smoothstep(-0.0005, 0.0005, altitudeAngle);
    float bloomVisible = smoothstep(-0.003, 0.003, altitudeAngle);

    disk *= diskVisible;
    bloom *= bloomVisible;

    float sunHeight = clamp(dot(sunDirWorld, -up), 0.0, 1.0);

float diskDayAmount  = smoothstep(0.02, 0.30, sunHeight);
float bloomDayAmount = smoothstep(-0.05, 0.18, sunHeight);

float sunsetFactor = 1.0 - smoothstep(0.0, 0.12, sunHeight);
sunsetFactor = pow(sunsetFactor, 2.0);

vec3 sunsetDiskTint  = vec3(1.0, 0.55, 0.28);
vec3 dayDiskTint     = vec3(1.0, 0.99, 0.97);

vec3 sunsetBloomTint = vec3(1.0, 0.65, 0.35);
vec3 dayBloomTint    = vec3(1.0, 0.98, 0.95);

vec3 sunDiskTint  = mix(sunsetDiskTint,  dayDiskTint,  diskDayAmount);
vec3 sunBloomTint = mix(sunsetBloomTint, dayBloomTint, bloomDayAmount);

// boost solo muy cerca del horizonte
sunDiskTint  = mix(sunDiskTint,  vec3(1.0, 0.35, 0.08), sunsetFactor);
sunBloomTint = mix(sunBloomTint, vec3(1.0, 0.45, 0.12), sunsetFactor);

    disk *= sunDiskTint;
    bloom *= sunBloomTint;

    vec3 sky = jodieReinhardTonemap(skyLum * AtmosphereExposure);

    vec3 sunRaw = disk * 4.0 + bloom * 6.0;
    vec3 sun = sunRaw / (1.0 + sunRaw * 0.30);

    vec3 finalColor = sky + sun;
    outColor = vec4(finalColor, 1.0);
}