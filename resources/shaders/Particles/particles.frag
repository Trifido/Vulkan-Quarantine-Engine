#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 2) uniform sampler2D Texture1;

void main() 
{
    //vec2 coord = gl_PointCoord - vec2(0.5);
    outColor = fragColor * texture(Texture1, texCoord);

    if(outColor.a == 0.0)
        discard;
}
