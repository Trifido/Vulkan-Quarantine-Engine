#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 currentTexCoord;
layout(location = 2) in vec2 nextTexCoord;
layout(location = 3) in float blendFactor;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 2) uniform sampler2D Texture1;

void main() 
{
    vec4 currentColor = texture(Texture1, currentTexCoord);
    vec4 nextColor = texture(Texture1, nextTexCoord);

    //vec2 coord = gl_PointCoord - vec2(0.5);
    outColor = mix(currentColor, nextColor, blendFactor) * fragColor;

    //if(outColor.a == 0.0)
    //    discard;
}
