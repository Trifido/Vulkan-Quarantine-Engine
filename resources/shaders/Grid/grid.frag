#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_OES_standard_derivatives : enable

layout(location = 1) in vec3 nearPoint;
layout(location = 2) in vec3 farPoint;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform ViewUniforms {
    mat4 view;
    mat4 proj;
} view;

vec4 grid(vec3 fragPos3D, float scale, bool drawAxis)
{
    vec2 coord = fragPos3D.xz * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);

    vec4 color = vec4(1.0 - min(line, 1.0));

    // z axis
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
    {
        color.rgba = vec4(0.0, 0.0, 1.0, 1.0);
    }
    // x axis
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
    {
        color.rgba = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
        if(drawAxis)
            color.rgba *= 0.9;
        else
            color.rgba *= 0.0;
    }

    return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = view.proj * view.view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float computeLinearDepth(vec3 pos) {
    float near = 0.1;
    float far = 100;
    vec4 clip_space_pos = view.proj * view.view * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far; // normalize
}

float getFogFactor(float d)
{
    const float FogMax = 20.0;
    const float FogMin = 10.0;

    if (d>=FogMax) return 1;
    if (d<=FogMin) return 0;

    return 1 - (FogMax - d) / (FogMax - FogMin);
}

void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    if(t <= 0)
        discard;

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));
    //vec4 V = fragPos3D;
    //float d = distance(CameraEye, V);
    //float alpha = getFogFactor(d);

    outColor = grid(fragPos3D, 10, true) + grid(fragPos3D, 1, false);
    outColor.a = fading;
}
