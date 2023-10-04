#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() 
{
    if(fragColor.a == 0.0)
        discard;

    vec2 coord = gl_PointCoord - vec2(0.5);
    outColor = fragColor; //vec4(fragColor.rgb, 0.5 - length(coord));
}
