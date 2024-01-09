#version 450
#extension GL_ARB_separate_shader_objects : enable

const int numberOfLights = 8;

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout(set = 0, binding = 1) uniform UniformMaterial {
    float Shininess;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
} uboMaterial;

struct LightData {
    vec4 position;
    vec3 diffuse;
    float constant;
    vec3 specular;
    float linear;
    vec3 spotDirection;
    float quadratic; 
    float spotCutoff;
    float spotExponent;
};

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
} uboLight;

layout(std140, binding = 3) buffer LightSSBO 
{
   LightData lights[];
};

layout(set = 0, binding = 4) uniform sampler2D texSampler[5];

//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, vec3 normal, vec2 texCoords);
vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec2 texCoords);
vec3 ComputeSpotLight(LightData light, vec3 normal, vec2 texCoords);

void main()
{
    vec3 normal = fs_in.Normal;

    if(uboMaterial.idxNormal > -1)
    {
        normal = texture(texSampler[uboMaterial.idxNormal], fs_in.TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * normal); 
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

    for(int i = 0; i < uboLight.numLights; i++)
    {
        if(lights[i].position.w == 1.0)
        {
            resultPoint += ComputePointLight(lights[i], normal, fs_in.TexCoords);
        }
        else if(lights[i].spotCutoff == 0.0)
        {
            resultDir += ComputeDirectionalLight(lights[i], normal, fs_in.TexCoords);
        }
        else
        {
            resultSpot += ComputeSpotLight(lights[i], normal, fs_in.TexCoords);
        }
    }

    result = resultPoint + resultDir + resultSpot;
    outColor = vec4(result, 1.0);
}

vec3 ComputePointLight(LightData light, vec3 normal, vec2 texCoords)
{
    float distance = length(cameraData.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse * attenuation;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}

vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position.xyz);

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}

vec3 ComputeSpotLight(LightData light, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);

    float distance = length(cameraData.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(normalize(light.position.xyz - fs_in.FragPos), normalize(-light.spotDirection)); 
    float epsilon = light.spotCutoff - light.spotExponent;
    float intensity = clamp((theta - light.spotExponent) / epsilon, 0.0, 1.0);

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse * intensity * attenuation;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular * intensity * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}
