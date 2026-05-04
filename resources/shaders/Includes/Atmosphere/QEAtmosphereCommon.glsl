#ifndef QE_Atmosphere_Common
#define QE_Atmosphere_Common

// Math
const float PI = 3.14159265358;

float safeAcos(float x)
{
    return acos(clamp(x, -1.0, 1.0));
}

// Compatibilidad con shaders que aún llaman safeacos
float safeacos(float x)
{
    return safeAcos(x);
}

// -----------------------------------------------------------------------------
// Atmosphere parameter accessors
// -----------------------------------------------------------------------------
float GetPlanetRadius()
{
    return atmosphereData.Planet_Atmosphere_G_Exposure.x;
}

float GetAtmosphereRadius()
{
    return atmosphereData.Planet_Atmosphere_G_Exposure.y;
}

float GetMieAnisotropy()
{
    return atmosphereData.Planet_Atmosphere_G_Exposure.z;
}

float GetExposure()
{
    return atmosphereData.Planet_Atmosphere_G_Exposure.w;
}

vec3 GetRayleighScatteringBase()
{
    return atmosphereData.RayleighScattering_Height.xyz;
}

float GetRayleighScaleHeight()
{
    return atmosphereData.RayleighScattering_Height.w;
}

vec3 GetMieScatteringBase()
{
    return atmosphereData.MieScattering_Height.xyz;
}

float GetMieScaleHeight()
{
    return atmosphereData.MieScattering_Height.w;
}

vec3 GetOzoneAbsorptionBase()
{
    return atmosphereData.OzoneAbsorption_Density.xyz;
}

float GetOzoneDensity()
{
    return atmosphereData.OzoneAbsorption_Density.w;
}

vec3 GetSunColor()
{
    return atmosphereData.SunColor_Intensity.xyz;
}

float GetSunIntensityMultiplier()
{
    return atmosphereData.SunColor_Intensity.w;
}

float GetSkyTint()
{
    return atmosphereData.Sky_Horizon_Multi_SunDisk.x;
}

float GetHorizonSoftness()
{
    return atmosphereData.Sky_Horizon_Multi_SunDisk.y;
}

float GetMultiScatteringFactor()
{
    return atmosphereData.Sky_Horizon_Multi_SunDisk.z;
}

float GetSunDiskSize()
{
    return atmosphereData.Sky_Horizon_Multi_SunDisk.w;
}

float GetSunDiskIntensity()
{
    return atmosphereData.SunDiskIntensity_Glow_Padding.x;
}

float GetSunGlow()
{
    return atmosphereData.SunDiskIntensity_Glow_Padding.y;
}

// -----------------------------------------------------------------------------
// Geometry helpers
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// Phase functions
// -----------------------------------------------------------------------------
float getMiePhase(float cosTheta)
{
    float g = GetMieAnisotropy();
    float scale = 3.0 / (8.0 * PI);

    float g2 = g * g;
    float num = (1.0 - g2) * (1.0 + cosTheta * cosTheta);
    float denom = (2.0 + g2) * pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);

    return scale * num / denom;
}

float getRayleighPhase(float cosTheta)
{
    const float k = 3.0 / (16.0 * PI);
    return k * (1.0 + cosTheta * cosTheta);
}

// -----------------------------------------------------------------------------
// Scattering model
// -----------------------------------------------------------------------------
void getScatteringValues(
    vec3 pos,
    out vec3 rayleighScattering,
    out float mieScattering,
    out vec3 extinction)
{
    float planetRadius = GetPlanetRadius();
    float rayleighScaleHeight = GetRayleighScaleHeight();
    float mieScaleHeight = GetMieScaleHeight();

    vec3 rayleighScatteringBase = GetRayleighScatteringBase();
    vec3 ozoneAbsorptionBase = GetOzoneAbsorptionBase();
    float ozoneDensity = GetOzoneDensity();

    float altitude = max(0.0, length(pos) - planetRadius);

    float rayleighDensity = exp(-altitude / max(rayleighScaleHeight, 1e-6));
    float mieDensity      = exp(-altitude / max(mieScaleHeight, 1e-6));

    rayleighScattering = rayleighScatteringBase * rayleighDensity;

    vec3 mieBaseVec = GetMieScatteringBase();
    float mieBase = (mieBaseVec.x + mieBaseVec.y + mieBaseVec.z) / 3.0;
    mieScattering = mieBase * mieDensity;

    float rayleighAbsorption = 0.0;
    float mieAbsorption = 4.4 * mieDensity;

    // 25 km -> 0.025 en unidades de shader
    // 15 km -> 0.015 en unidades de shader
    vec3 ozoneAbsorption = ozoneAbsorptionBase
        * ozoneDensity
        * max(0.0, 1.0 - abs(altitude - 0.025) / 0.015);

    extinction =
        rayleighScattering +
        rayleighAbsorption +
        vec3(mieScattering) +
        mieAbsorption +
        ozoneAbsorption;
}

#endif // QE_Atmosphere_Common