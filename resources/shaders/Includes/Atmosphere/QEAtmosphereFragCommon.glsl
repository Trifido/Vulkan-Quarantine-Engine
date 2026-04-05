#ifndef QE_Atmosphere_FragCommon
#define QE_Atmosphere_FragCommon

const float AtmosphereExposure = 12.0;
const float HorizonBias = -0.0028;

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

#endif // QE_Atmosphere_FragCommon