#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

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

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout(set = 0, binding = 4) uniform UniformMaterial {
    float Shininess;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
} uboMaterial;

layout(set = 0, binding = 5) uniform UniformManagerLight
{
    int numLights;
	LightData lights[8];
} uboLight;

layout(set = 0, binding = 6) uniform sampler2D texSampler[5];

layout(std430, push_constant) uniform PushConstants
{
    mat4 model;
} constants;

vec3 getNormalFromMap(vec2 TexCoords);

//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 viewDir, vec3 tangentLightPos);
vec3 ComputeDirectionalLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 viewDir);
vec3 ComputeSpotLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 viewDir, vec3 tangentLightPos);

void main()
{
    vec2 texCoords = inTexCoord;

    vec3 normal = inNormal;
    if(uboMaterial.idxNormal > -1)
    {
        normal = getNormalFromMap(texCoords);
    }

    //COMPUTE LIGHT
    vec3 result = vec3(1.0, 0.0, 0.5);

    if(uboMaterial.idxDiffuse > -1)
    {
        result = texture(texSampler[uboMaterial.idxDiffuse], inTexCoord).rgb;
    }

    //COMPUTE TANGENT SPACE DATA
    mat4 normalMatrix = transpose(inverse(constants.model));
    
    mat3 TBN = transpose(mat3(inTangent, inBitangent, inNormal));
    vec3 tangentViewPos = TBN * cameraData.position.xyz;
    vec3 tangentFragPos = TBN * inPosition;

    //COMPUTE VIEW DIRECTION IN TANGENT SPACE
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);

    vec3 resultPoint = vec3(0.0);
    vec3 resultDir = vec3(0.0);
    vec3 resultSpot = vec3(0.0);

    for(int i = 0; i < uboLight.numLights; i++)
    {
        if(uboLight.lights[i].position.w == 1.0)
        {            
            vec3 dirLight = TBN * uboLight.lights[i].position.xyz;
            dirLight = normalize(dirLight - tangentFragPos);
            resultPoint += ComputePointLight(uboLight.lights[i], i, normal, texCoords, dirLight, viewDir);
        }
        else if(uboLight.lights[i].spotCutoff == 0.0)
        {
            resultDir += ComputeDirectionalLight(uboLight.lights[i], i, normal, texCoords, viewDir);
        }
        else
        {            
            vec3 dirLight = TBN * uboLight.lights[i].position.xyz;
            dirLight = normalize(dirLight - tangentFragPos);
            resultSpot += ComputeSpotLight(uboLight.lights[i], i, normal, texCoords, dirLight, viewDir);
        }
    }

    result = resultPoint + resultDir + resultSpot;
    outColor = vec4(result, 1.0);
}

vec3 ComputePointLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 dirLight, vec3 viewDir)
{
    float distance = length(cameraData.position.xyz - inPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    float diff = max(dot(dirLight, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse * attenuation;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 reflectDir = reflect(-dirLight, normal);
    vec3 halfwayDir = normalize(dirLight + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return (ambient + diffuse + specular + emissive);
}

vec3 ComputeDirectionalLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 viewDir)
{
    vec3 dirLight = normalize(light.position.xyz);

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    float diff = max(dot(dirLight, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 reflectDir = reflect(-dirLight, normal);
    vec3 halfwayDir = normalize(dirLight + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return (ambient + diffuse + specular + emissive);
}


vec3 ComputeSpotLight(LightData light, int id, vec3 normal, vec2 texCoords, vec3 dirLight, vec3 viewDir)
{
    float distance = length(cameraData.position.xyz - inPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(normalize(light.position.xyz - inPosition), normalize(-light.spotDirection)); 
    float epsilon = light.spotCutoff - light.spotExponent;
    float intensity = clamp((theta - light.spotExponent) / epsilon, 0.0, 1.0);

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    float diff = max(dot(normal, dirLight), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse * intensity * attenuation;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 reflectDir = reflect(-dirLight, normal);
    vec3 halfwayDir = normalize(dirLight + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specular = spec * light.specular * colorSpecular * intensity * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return (ambient + diffuse + specular + emissive);
}


vec3 getNormalFromMap(vec2 TexCoords)
{
    vec3 tangentNormal = texture(texSampler[uboMaterial.idxNormal], TexCoords).xyz;
    return normalize(tangentNormal * 2.0 - 1.0);
}
