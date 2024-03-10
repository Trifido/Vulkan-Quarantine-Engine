#version 450
#extension GL_ARB_separate_shader_objects : enable

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

#define TILE_SIZE 8
#define NUM_BINS 16.0
#define BIN_WIDTH ( 1.0 / NUM_BINS )
#define MAX_NUM_LIGHTS 64
#define NUM_WORDS ( ( MAX_NUM_LIGHTS + 31 ) / 32 )

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

struct LightData 
{
    vec3 position;
    uint lightType;
    vec3 diffuse;
    float constant;
    vec3 specular;
    float linear;
    vec3 spotDirection;
    float quadratic; 
    float cutoff;
    float outerCutoff;
    float radius;
    uint idxShadowMap;
};

layout(set = 0, binding = 0) uniform CameraUniform
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
    vec4 position;
    vec4 frustumPlanes[6];
} cameraData;

layout(set = 0, binding = 1) uniform UniformMaterial 
{
    vec4 Diffuse;
    vec4 Ambient;
    vec4 Specular;
    vec4 Emissive;
    vec4 Transparent;
    vec4 Reflective;

    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;

    float Opacity;
    float BumpScaling;
    float Reflectivity;
    float Refractivity;
    float Shininess;
    float Shininess_Strength;
} uboMaterial;

layout(set = 0, binding = 2) uniform UniformManagerLight
{
    int numLights;
} uboLight;

layout(set = 0, binding = 3) readonly buffer LightSSBO 
{
   LightData lights[];
};

layout(set = 0, binding = 4) readonly buffer LightIndices 
{
    uint light_indices[];
};

layout(set = 0, binding = 5) readonly buffer ZBins 
{
    uint bins[];
};

layout(set = 0, binding = 6) readonly buffer Tiles 
{
    uint tiles[];
};

layout(set = 0, binding = 7) uniform sampler2D texSampler[5];

layout(set = 0, binding = 8) uniform ScreenData 
{
    vec2 resolution;
} screenData;

//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeSpotLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeAmbientLight(vec3 diffuseColor);
vec3 GetAlbedoColor();
vec3 GetSpecularColor();
vec3 GetEmissiveColor();

void main()
{
    vec4 pos_camera_space = cameraData.view * vec4(fs_in.FragPos, 1.0);
    float z_light_far = 500.0;
    float z_near = 0.1;
    float linear_d = (-pos_camera_space.z - z_near) / (z_light_far - z_near);
    int bin_index = int( linear_d / BIN_WIDTH );
    uint bin_value = bins[ bin_index ];

    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = ( bin_value >> 16 ) & 0xFFFF;

    uvec2 pix_tile_size = uvec2(screenData.resolution / TILE_SIZE);
    uvec2 tile = uvec2(gl_FragCoord.xy / pix_tile_size);
    uint stride = uint( NUM_WORDS ) * pix_tile_size.x;
    uint address = tile.y * stride + tile.x;

    vec3 normal = fs_in.Normal;

    if(uboMaterial.idxNormal > -1)
    {
        normal = texture(texSampler[uboMaterial.idxNormal], fs_in.TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * normal) * uboMaterial.BumpScaling; 
    }

    //COMPUTE LIGHT
    vec3 albedoColor = GetAlbedoColor();
    vec3 specularColor = GetSpecularColor();
    vec3 emissiveColor = GetEmissiveColor();
    vec3 result = ComputeAmbientLight(albedoColor);

    vec3 resultPoint = vec3(0.0);
    vec3 resultDir = vec3(0.0);
    vec3 resultSpot = vec3(0.0);
    
    if (min_light_id != uboLight.numLights + 1)
    {
        for (uint light_id = min_light_id; light_id <= max_light_id; ++light_id) 
        {
            uint word_id = light_id / 32;
            uint bit_id = light_id % 32;

            if ((tiles[ address + word_id ] & (1 << bit_id)) != 0) 
            {
                uint gli = light_indices[light_id];

                if (lights[gli].lightType == POINT_LIGHT)
                {
                    resultPoint += ComputePointLight(lights[gli], normal, albedoColor, specularColor, emissiveColor);
                }
                else if (lights[gli].lightType == DIRECTIONAL_LIGHT)
                {
                    resultDir += ComputeDirectionalLight(lights[gli], normal, albedoColor, specularColor, emissiveColor);
                }
                else
                {
                    resultSpot += ComputeSpotLight(lights[gli], normal, albedoColor, specularColor, emissiveColor);
                }
            }
        }  
    }

    result += resultPoint + resultDir + resultSpot;
    outColor = vec4(result, uboMaterial.Opacity);
}

vec3 GetAlbedoColor()
{
    vec3 colorDiffuse = uboMaterial.Diffuse.rgb;
    if(uboMaterial.idxDiffuse > -1)
        colorDiffuse = vec3(texture(texSampler[uboMaterial.idxDiffuse], fs_in.TexCoords));

    return colorDiffuse;
}

vec3 GetSpecularColor()
{
    vec3 colorSpecular = uboMaterial.Specular.rgb;
    if(uboMaterial.idxSpecular > -1)
        colorSpecular = vec3(texture(texSampler[uboMaterial.idxSpecular], fs_in.TexCoords));
    return colorSpecular;
}

vec3 GetEmissiveColor()
{
    vec3 emissive = uboMaterial.Emissive.rgb;
    if(uboMaterial.idxEmissive > -1)
        emissive = vec3(texture(texSampler[uboMaterial.idxEmissive], fs_in.TexCoords));
    return emissive;
}

vec3 ComputeAmbientLight(vec3 diffuseColor)
{
    return diffuseColor * uboMaterial.Ambient.rgb; //0.1 * 
}

vec3 ComputePointLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    float distance = length(light.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 lightDir = normalize(light.position - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * albedo * attenuation;

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specularResult = spec * light.specular * specular * attenuation;

    return diffuse + specularResult + emissive;
}

vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    vec3 lightDir = normalize(light.position);

    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * albedo;

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specularResult = spec * light.specular * specular;

    return diffuse + specularResult + emissive;
}

vec3 ComputeSpotLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);

    float distance = length(light.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.spotDirection)); 
    float epsilon = light.cutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseResult = diff * light.diffuse * albedo * intensity * attenuation;

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specularResult = spec * light.specular * specular * intensity * attenuation;

    return diffuseResult + specularResult + emissive;
}
