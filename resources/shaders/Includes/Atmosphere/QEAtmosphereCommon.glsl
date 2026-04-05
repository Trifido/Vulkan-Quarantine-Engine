#ifndef QE_Atmosphere_Common
#define QE_Atmosphere_Common

// Math
const float PI = 3.14159265358;

// Atmosphere constants
const float PlanetRadius = 6.360;
const float AtmosphereRadius = 6.460;

// Scattering coefficients
const vec3 rayleighScatteringBase = vec3(5.802, 13.558, 33.1);
const float mieScatteringBase = 3.996;
const float mieExtinctionBase = 4.4;
const float rayleighAbsorptionBase = 0.0;
const vec3 ozoneAbsorptionBase = vec3(0.650, 1.881, 0.085);

// Optional named scale heights already used in some shaders
const float kHeightR = 8.0;
const float kHeightM = 1.2;

float safeAcos(float x)
{
    return acos(clamp(x, -1.0, 1.0));
}

// Compatibilidad con shaders que aún llaman safeacos
float safeacos(float x)
{
    return safeAcos(x);
}

// From https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code.
float rayIntersectSphere(vec3 ro, vec3 rd, float rad)
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - rad * rad;
    if (c > 0.0 && b > 0.0) return -1.0;
    float discr = b * b - c;
    if (discr < 0.0) return -1.0;

    if (discr > b * b) return (-b + sqrt(discr));
    return -b - sqrt(discr);
}

// From https://www.shadertoy.com/view/wlBXWK
vec2 rayIntersectSphere2D(vec3 start, vec3 dir, float radius)
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (radius * radius);
    float d = (b * b) - 4.0 * a * c;

    if (d < 0.0) return vec2(1e5, -1e5);

    return vec2(
        (-b - sqrt(d)) / (2.0 * a),
        (-b + sqrt(d)) / (2.0 * a)
    );
}

float getMiePhase(float cosTheta)
{
    const float g = 0.8;
    const float scale = 3.0 / (8.0 * PI);

    float num = (1.0 - g * g) * (1.0 + cosTheta * cosTheta);
    float denom = (2.0 + g * g) * pow((1.0 + g * g - 2.0 * g * cosTheta), 1.5);

    return scale * num / denom;
}

float getRayleighPhase(float cosTheta)
{
    const float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cosTheta * cosTheta);
}

void getScatteringValues(
    vec3 pos,
    out vec3 rayleighScattering,
    out float mieScattering,
    out vec3 extinction)
{
    float altitudeKM = (length(pos) - PlanetRadius) * 1000.0;
    float rayleighDensity = exp(-altitudeKM / kHeightR);
    float mieDensity = exp(-altitudeKM / kHeightM);

    rayleighScattering = rayleighScatteringBase * rayleighDensity;
    float rayleighAbsorption = rayleighAbsorptionBase * rayleighDensity;

    mieScattering = mieScatteringBase * mieDensity;
    float mieAbsorption = mieExtinctionBase * mieDensity;

    vec3 ozoneAbsorption = ozoneAbsorptionBase * max(0.0, 1.0 - abs(altitudeKM - 25.0) / 15.0);

    extinction = rayleighScattering + rayleighAbsorption + mieScattering + mieAbsorption + ozoneAbsorption;
}

#endif // QE_Atmosphere_Common