#version 450
#extension GL_ARB_separate_shader_objects : enable

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

#define TILE_SIZE 16
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
    float spotCutoff;
    float spotExponent;
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

layout(set = 0, binding = 1) uniform UniformMaterial {
    float Shininess;
    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxEmissive;
    int idxHeight;
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
vec3 ComputePointLight(LightData light, vec3 normal, vec2 texCoords);
vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec2 texCoords);
vec3 ComputeSpotLight(LightData light, vec3 normal, vec2 texCoords);

void main()
{
    uint test = light_indices[0];
    uint test1 = bins[0];
    uint test2 = tiles[0];

    vec4 pos_camera_space = cameraData.view * vec4(fs_in.FragPos, 1.0);
    float z_light_far = 100.0;
    float z_near = 0.01;
    float linear_d = (-pos_camera_space.z - z_near) / (z_light_far - z_near);
    int bin_index = int( linear_d / BIN_WIDTH );
    uint bin_value = bins[ bin_index ];

    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = ( bin_value >> 16 ) & 0xFFFF;

    uvec2 position = uvec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);
    position.y = uint( screenData.resolution.y ) - position.y;

    uvec2 tile = position / uint( TILE_SIZE );
    uint stride = uint( NUM_WORDS ) * ( uint( screenData.resolution.x ) / uint( TILE_SIZE ) );
    uint address = tile.y * stride + tile.x;

    //------------------------------------------------------------------------------------

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

    //-----------------------------------------------------------------------------------

    if( min_light_id != uboLight.numLights + 1 )
    {
        for (uint light_id = min_light_id; light_id <= max_light_id; ++light_id) 
        {
            uint word_id = light_id / 32;
            uint bit_id = light_id % 32;

            if ( ( tiles[ address + word_id ] & ( 1 << bit_id ) ) != 0 ) 
            {
                uint global_light_index = light_indices[light_id];
                resultPoint += ComputePointLight(lights[global_light_index], normal, fs_in.TexCoords);
            }
        }  
    }

    // for(int i = 0; i < uboLight.numLights; i++)
    // {
    //     if(lights[i].lightType == POINT_LIGHT)
    //     {
    //         resultPoint += ComputePointLight(lights[i], normal, fs_in.TexCoords);
    //     }
    //     else if(lights[i].lightType == DIRECTIONAL_LIGHT)
    //     {
    //         resultDir += ComputeDirectionalLight(lights[i], normal, fs_in.TexCoords);
    //     }
    //     else
    //     {
    //         resultSpot += ComputeSpotLight(lights[i], normal, fs_in.TexCoords);
    //     }
    // }

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

    vec3 lightDir = normalize(light.position - fs_in.FragPos);
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
    return ambient + diffuse + specular + emissive;
}

vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position);

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
    return ambient + diffuse + specular + emissive;
}

vec3 ComputeSpotLight(LightData light, vec3 normal, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position.xyz - fs_in.FragPos);

    float distance = length(cameraData.position.xyz - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(normalize(light.position - fs_in.FragPos), normalize(-light.spotDirection)); 
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
    return ambient + diffuse + specular + emissive;
}
