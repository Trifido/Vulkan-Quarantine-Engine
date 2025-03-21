#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT 
{
    vec2 TexCoords;
} QE_in;

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

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265358;

// Unidades en Megametros (MM)
const float groundRadiusMM = 6.360;
const float atmosphereRadiusMM = 6.460;

const vec3 viewPos = vec3(0.0, groundRadiusMM + 0.0002, 0.0);

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
    float sunCosZenithAngle = dot(sunDir, up);
    
    vec2 uv = vec2(
        tLUTRes.x * clamp(0.5 + 0.5 * sunCosZenithAngle, 0.0, 1.0),
        tLUTRes.y * max(0.0, min(1.0, (height - groundRadiusMM) / (atmosphereRadiusMM - groundRadiusMM)))
    );
    uv /= tLUTRes;
    
    ivec2 texelCoords = ivec2(uv * tLUTRes);
    return texture(InputImage, texelCoords).rgb;
}

const vec2 skyLUTRes = vec2(200.0, 200.0);
vec3 getValFromSkyLUT(vec3 rayDir, vec3 sunDir)
{
    float height = length(viewPos);
    vec3 up = viewPos / height;
    
    float horizonAngle = safeAcos(sqrt(height * height - groundRadiusMM * groundRadiusMM) / height);
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
    uv /= skyLUTRes;
    return texture(InputImage_2, uv).rgb;
}

vec3 jodieReinhardTonemap(vec3 c)
{
    // From: https://www.shadertoy.com/view/tdSXzD
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.0);
    return mix(c / (l + 1.0), tc, tc);
}

vec3 sunWithBloom(vec3 rayDir, vec3 sunDir) {
    const float sunSolidAngle = 0.53*PI/180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(rayDir, sunDir);
    if (cosTheta >= minSunCosTheta) return vec3(1.0);
    
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset*50000.0)*0.5;
    float invBloom = 1.0/(0.02 + offset*300.0)*0.01;
    return vec3(gaussianBloom+invBloom);
}

float getSunAltitude(float time)
{
    const float periodSec = 120.0;
    const float halfPeriod = periodSec / 2.0;
    const float sunriseShift = 0.1;
    float cyclePoint = (1.0 - abs((mod(time,periodSec)-halfPeriod)/halfPeriod));
    cyclePoint = (cyclePoint*(1.0+sunriseShift))-sunriseShift;
    return (0.5*PI)*cyclePoint;
}

vec3 getSunDir()
{
    float altitude = getSunAltitude(10.5);
    return normalize(vec3(0.0, sin(altitude), -cos(altitude)));
}

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;
    vec3 sunDir = getSunDir();

    vec4 ndc = vec4(QE_in.TexCoords * 2.0 - 1.0, 1.0, 1.0); // (x,y) en [-1,1], z=1 para dirección
    mat4 invProj = inverse(cameraData.proj);
    mat4 invView = inverse(cameraData.view);
    vec4 viewDir = invProj * ndc;  // Convertir a espacio de cámara
    viewDir /= viewDir.w;          // Hacer homogéneo

    vec3 camDir = normalize((invView * vec4(viewDir.xyz, 0.0)).xyz); // Pasar a espacio mundial

    //vec3 camDir = normalize(vec3(0.0, 0.27, -1.0));
    float camFOVWidth = PI/3.5;
    float camWidthScale = 2.0*tan(camFOVWidth/2.0);
	vec2 iResolution = vec2(1080, 720);
    float camHeightScale = camWidthScale*iResolution.y/iResolution.x;
    
    vec3 camRight = normalize(cross(camDir, vec3(0.0, 1.0, 0.0)));
    vec3 camUp = normalize(cross(camRight, camDir));

    // Convertir dirección a coordenadas esféricas
    float phi = atan(camDir.z, camDir.x);
    float u = phi / (2.0 * 3.14159265359);
    
    float theta = asin(camDir.y);
    float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (3.14159265359 / 2.0));
    
    vec2 xy = vec2(u, v);
    vec3 rayDir = normalize(camDir + camRight*xy.x*camWidthScale + camUp*xy.y*camHeightScale);
    
    vec3 lum = getValFromSkyLUT(rayDir, sunDir);

    // Bloom should be added at the end, but this is subtle and works well.
    vec3 sunLum = sunWithBloom(rayDir, sunDir);
    // Use smoothstep to limit the effect, so it drops off to actual zero.
    sunLum = smoothstep(0.002, 1.0, sunLum);
    if (length(sunLum) > 0.0) {
        if (rayIntersectSphere(viewPos, rayDir, groundRadiusMM) >= 0.0) {
            sunLum *= 0.0;
        } else {
            // If the sun value is applied to this pixel, we need to calculate the transmittance to obscure it.
            sunLum *= getValFromTLUT(viewPos, sunDir);
        }
    }
    lum += sunLum;
    
    // Tonemapping and gamma. Super ad-hoc, probably a better way to do this.
    lum *= 20.0;
    lum = pow(lum, vec3(1.3));
    lum /= (smoothstep(0.0, 0.2, clamp(sunDir.y, 0.0, 1.0))*2.0 + 0.15);
    
    lum = jodieReinhardTonemap(lum);
    
    lum = pow(lum, vec3(1.0/2.2));
    
    outColor = vec4(lum,1.0);
}