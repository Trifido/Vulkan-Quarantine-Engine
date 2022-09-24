#version 450
#extension GL_ARB_separate_shader_objects : enable

const int numberOfLights = 8;

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec3 position;
} cameraData;

layout(set = 0, binding = 1) uniform UniformMaterial {
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
    int idxBump;
    float shininess;
} uboMaterial;

struct LightData {
    vec4 position;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic; 
};

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
	LightData lights[8];
} uboLight;

layout(set = 0, binding = 3) uniform sampler2D texSampler[6];


//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, vec3 normal, vec3 viewDir, vec2 texCoords);

void main()
{
    //GET VIEW DIRECTION  
    vec3 viewDir = normalize(cameraData.position.xyz - fs_in.FragPos);

    //GET TEXTURE COORDS
    vec2 texCoords = fs_in.TexCoords;
    if(texture(texSampler[uboMaterial.idxDiffuse], texCoords).a < 0.1)
        discard;

    vec3 normal = fs_in.Normal;

    if(uboMaterial.idxNormal > -1)
    {
        normal = texture(texSampler[uboMaterial.idxNormal], texCoords).rgb;
        normal = normalize(normal * 2.0 - 1.0);
    }

    //COMPUTE LIGHT
    vec3 result = vec3(1.0, 0.0, 0.5);

    if(uboMaterial.idxDiffuse > -1)
    {
        result = texture(texSampler[uboMaterial.idxDiffuse], fs_in.TexCoords).rgb;
    }

    vec3 resultPoint = vec3(0.0);
    vec3 resultDir = vec3(0.0);
    vec3 resultSpot = vec3(0.0);
    vec3 resultFPS = vec3(0.0);

    for(int i = 0; i < uboLight.numLights; i++)
    {
        if(uboLight.lights[i].position.w == 1.0)
        {
            resultPoint += ComputePointLight(uboLight.lights[i], normal, viewDir, texCoords);
        }
    }

    result *= (resultPoint + resultDir + resultSpot);
    outColor = vec4(result, 1.0);
}

vec3 ComputePointLight(LightData light, vec3 normal, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.shininess);

    float distance = length(light.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // - DIFFUSE
    vec3 resultDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        resultDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    // - SPECULAR
    vec3 resultSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        resultSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    // - EMISSIVE
    vec3 resultEmissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        resultEmissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // combine results
    vec3 ambient = resultDiffuse * vec3(0.3, 0.24, 0.14);
    vec3 diffuse  = light.diffuse * diff * resultDiffuse;// * attenuation;
    vec3 specular = light.specular * spec * resultSpecular;// * attenuation;
    vec3 emissive = resultEmissive;
    return ((ambient + diffuse + specular + emissive));
}
