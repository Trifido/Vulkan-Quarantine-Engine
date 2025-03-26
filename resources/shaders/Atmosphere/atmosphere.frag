#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D InputImage;
layout(binding = 1) uniform sampler2D InputImage_2;

layout(set = 0, binding = 2) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout(set = 0, binding = 3) uniform ScreenResolution
{
	vec2 data;
} screenResolution;

layout(set = 0, binding = 4) uniform SunUniform
{
	vec3 direction;
    float intensity;
}  sunData;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358;

// Unidades en Megametros (MM)
const float PlanetRadius = 6.360;
const float AtmosphereRadius = 6.460;

float safeAcos(float x) 
{
    return acos(clamp(x, -1.0, 1.0));
}

float rayIntersectSphere(vec3 ro, vec3 rd, float radius) 
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float delta = b * b - c;
    if (delta < 0.0) return -1.0; // No hay intersección

    float sqrtDelta = sqrt(delta);
    float t1 = -b - sqrtDelta;
    float t2 = -b + sqrtDelta;

    if (t1 > 0.0) return t1; // Primer impacto
    if (t2 > 0.0) return t2; // Segundo impacto (si el origen está dentro)
    return -1.0; // No hay intersección válida
}

const vec2 tLUTRes = vec2(256.0, 64.0);
vec3 getValFromTLUT(vec3 pos, vec3 sunDir)
{
    float height = length(pos);
    vec3 up = pos / height;
    float sunCosZenithAngle = dot(sunDir, -up);
    
    vec2 uv = vec2(
        tLUTRes.x * clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0),
        tLUTRes.y * max(0.0, min(1.0, (height - PlanetRadius) / (AtmosphereRadius - PlanetRadius)))
    );
    uv /= tLUTRes;
    
    return texture(InputImage, uv).rgb;
}

const vec2 skyLUTRes = vec2(640.0, 360.0);
vec3 getValFromSkyLUT(vec3 viewPos, vec3 rayDir, vec3 sunDir)
{
    float height = length(viewPos);
    vec3 up = viewPos / height;
    
    float horizonAngle = safeAcos(sqrt(height * height - PlanetRadius * PlanetRadius) / height);
    float altitudeAngle = horizonAngle - acos(dot(rayDir, up)); // Between -PI/2 and PI/2
    float azimuthAngle; // Between 0 and 2*PI
    if (abs(altitudeAngle) > (0.5*PI - 0.0001)) {
        // Looking nearly straight up or down.
        azimuthAngle = 0.0;
    } else {
        vec3 right = cross(sunDir, up);
        vec3 forward = cross(up, right);
        
        vec3 projectedDir = normalize(rayDir - up*(dot(rayDir, up)));
        float sinTheta = dot(projectedDir, right);
        float cosTheta = dot(projectedDir, forward);
        azimuthAngle = atan(sinTheta, cosTheta) + PI;
    }
    
    // Non-linear mapping of altitude angle. See Section 5.3 of the paper.
    float v = 0.5 + 0.5*sign(altitudeAngle)*sqrt(abs(altitudeAngle)*2.0/PI);
    vec2 uv = vec2(azimuthAngle / (2.0*PI), v);

    return texture(InputImage_2, uv).rgb;
}

vec3 jodieReinhardTonemap(vec3 c)
{
    // From: https://www.shadertoy.com/view/tdSXzD
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.0);
    return mix(c / (l + 1.0), tc, tc);
}

vec3 sunWithBloom(vec3 rayDir, vec3 sunDir)
{
    const float sunSolidAngle = 0.53*PI/180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(rayDir, sunDir);
    if (cosTheta >= minSunCosTheta) return vec3(1.0);
    
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset*50000.0)*0.5;
    float invBloom = 1.0/(0.02 + offset*300.0)*0.01;
    return vec3(gaussianBloom+invBloom);
}

void main()
{
    vec3 sunDir = normalize(sunData.direction);
    vec2 iResolution = screenResolution.data;
    vec3 viewPos = cameraData.position.xyz * 1e-6;
    viewPos.y += PlanetRadius;
    viewPos.y = max(PlanetRadius + 1e-6, viewPos.y);

    vec3 camDir = normalize(cameraData.view[2].xyz);
    float camFOVWidth = 0.785398;
    float camWidthScale = 2.0 * tan(camFOVWidth / 2.0);
    float camHeightScale = camWidthScale * iResolution.y / iResolution.x;

    vec3 upVector = vec3(0.0, 1.0, 0.0);
    vec3 camRight = normalize(cross(camDir, upVector));
    vec3 camUp = cross(camRight, camDir);

    vec2 xy = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    xy.x= -xy.x;
    
    vec3 rayDirView = normalize(vec3(xy.x * camWidthScale, (xy.y + 0.01) * camHeightScale, 1.0));
    mat4 viewToWorld = inverse(cameraData.view);
    // Convertir la dirección del rayo a espacio mundial
    vec3 rayDir = normalize((viewToWorld * vec4(rayDirView, 0.0)).xyz);
    
    vec3 lum = getValFromSkyLUT(viewPos, rayDir, sunDir);

    // Bloom should be added at the end, but this is subtle and works well.
    vec3 sunLum = sunWithBloom(rayDir, sunDir);
    // Use smoothstep to limit the effect, so it drops off to actual zero.
    sunLum = smoothstep(0.002, 1.0, sunLum);
    if (length(sunLum) > 0.0) 
    {
        if (rayIntersectSphere(viewPos, rayDir, PlanetRadius) < 0.0)
        {
            sunLum *= 0.0;
        } 
        else 
        {
            // If the sun value is applied to this pixel, we need to calculate the transmittance to obscure it.
            sunLum *= getValFromTLUT(viewPos, sunDir);
        }
    }

    lum += sunLum;
    lum *= sunData.intensity;

    lum = jodieReinhardTonemap(lum);
    lum = pow(lum, vec3(1.0/2.2));
    
    outColor = vec4(lum, 1.0);
}