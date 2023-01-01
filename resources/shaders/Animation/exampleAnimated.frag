#version 450
#extension GL_ARB_separate_shader_objects : enable

const int numberOfLights = 8;

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec3 TangentLightPos[8];
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
    float shininess;
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
	LightData lights[8];
} uboLight;

layout(set = 0, binding = 4) uniform sampler2D texSampler[6];

vec3 getNormalFromMap(vec2 TexCoords);

//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, int id, vec3 normal, vec2 texCoords);
vec3 ComputeDirectionalLight(LightData light, int id, vec3 normal, vec2 texCoords);
vec3 ComputeSpotLight(LightData light, int id, vec3 normal, vec2 texCoords);

void main()
{
    //GET VIEW DIRECTION  
    //vec3 viewDir = normalize(cameraData.position.xyz - fs_in.FragPos);

    //GET TEXTURE COORDS
    vec2 texCoords = fs_in.TexCoords;
    //if(texture(texSampler[uboMaterial.idxDiffuse], texCoords).a < 0.1)
    //    discard;

    vec3 normal = fs_in.Normal;

    if(uboMaterial.idxNormal > -1)
    {
        normal = getNormalFromMap(texCoords);
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
        if(uboLight.lights[i].position.w == 1.0)
        {
            resultPoint += ComputePointLight(uboLight.lights[i], i, normal, texCoords);
        }
        else if(uboLight.lights[i].spotCutoff == 0.0)
        {
            resultDir += ComputeDirectionalLight(uboLight.lights[i], i, normal, texCoords);
        }
        else
        {
            resultSpot += ComputeSpotLight(uboLight.lights[i], i, normal, texCoords);
        }
    }

    result = resultPoint + resultDir + resultSpot;
    outColor = vec4(result, 1.0);
}

vec3 ComputePointLight(LightData light, int id, vec3 normal, vec2 texCoords)
{
    float distance = length(fs_in.TangentViewPos - fs_in.TangentFragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // - DIFFUSE
    vec3 colorDiffuse = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], texCoords));

    vec3 lightDir = normalize(fs_in.TangentLightPos[id] - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * colorDiffuse * attenuation;

    // - AMBIENT
    vec3 ambient = 0.1 * colorDiffuse;

    // - SPECULAR
    vec3 colorSpecular = vec3 (1.0, 1.0, 1.0);
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], texCoords));

    vec3 view_dir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.shininess);
    vec3 specular = spec * light.specular * colorSpecular * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}

vec3 ComputeDirectionalLight(LightData light, int id, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(fs_in.TangentLightPos[id]);

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

    vec3 view_dir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.shininess);
    vec3 specular = spec * light.specular * colorSpecular;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}


vec3 ComputeSpotLight(LightData light, int id, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(fs_in.TangentLightPos[id] - fs_in.TangentFragPos);

    float distance = length(cameraData.position - fs_in.FragPos);
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

    vec3 view_dir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.shininess);
    vec3 specular = spec * light.specular * colorSpecular * intensity * attenuation;

    // - EMISSIVE
    vec3 emissive = vec3 (0.0, 0.0, 0.0);
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], texCoords));

    // -- RESULT --
    return ((ambient + diffuse + specular + emissive));
}


vec3 getNormalFromMap(vec2 TexCoords)
{
    vec3 tangentNormal = texture(texSampler[uboMaterial.idxNormal], TexCoords).xyz;
    return normalize(tangentNormal * 2.0 - 1.0);
}
