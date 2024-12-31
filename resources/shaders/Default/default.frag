#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

#define CSM_COUNT 4

#define EPSILON 0.15
#define SHADOW_OPACITY 0.5

#define NUM_BINS 16.0
#define BIN_WIDTH ( 1.0 / NUM_BINS )
#define MAX_NUM_LIGHTS 64
#define NUM_WORDS ( ( MAX_NUM_LIGHTS + 31 ) / 32 )
#define FAR_PLANE 500.0;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
); 

layout(location = 0) in VS_OUT {
    vec3 FragPos;
    vec3 ViewPos;
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
    vec3 direction;
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
    uvec2 pix_tile_size;
} screenData;

layout(set = 1, binding = 0) uniform samplerCube QE_PointShadowCubemaps[10];

layout(set = 2, binding = 0) uniform sampler2DArray QE_DirectionalShadowmaps[10];

layout (set = 2, binding = 1) uniform UniformCSM
{
    vec4 Splits[10];
} QE_Cascade;

layout (set = 2, binding = 2) readonly buffer cascadeViewProjs
{
    mat4 QE_CascadeViewProj[];
};

//BLINN-PHONG LIGHT EQUATIONS
vec3 ComputePointLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeSpotLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive);
vec3 ComputeAmbientLight(vec3 diffuseColor);

vec3 GetAlbedoColor();
vec3 GetSpecularColor();
vec3 GetEmissiveColor();

float ComputeCSMTextureProj(vec4 shadowCoord, vec2 offset, uint lightIdx, uint cascadeIndex);
float ComputeCSMFilterPCF(vec4 sc, uint lightIdx, uint cascadeIndex);
float ComputeCubeMapPCF(uint lightIdx, vec3 lightVec, float distance);

void main()
{
    vec4 pos_camera_space = cameraData.view * vec4(fs_in.FragPos, 1.0);
    float z_far = FAR_PLANE;
    float z_near = 0.1;
    float linear_d = (-pos_camera_space.z - z_near) / (z_far - z_near);
    int bin_index = int( linear_d / BIN_WIDTH );
    uint bin_value = bins[ bin_index ];

    uint min_light_id = bin_value & 0xFFFF;
    uint max_light_id = ( bin_value >> 16 ) & 0xFFFF;

    uvec2 tile = uvec2(gl_FragCoord.xy / screenData.pix_tile_size);
    uint stride = uint( NUM_WORDS ) * screenData.pix_tile_size.x;
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
    return diffuseColor * uboMaterial.Ambient.rgb;
}

vec3 ComputePointLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    vec3 lightDirVector = light.position - fs_in.FragPos;
    float distance = length(lightDirVector);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 lightDir = normalize(lightDirVector);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * albedo * attenuation;

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specularResult = spec * light.specular * specular * attenuation;

    vec3 result = diffuse + specularResult + emissive;

    float shadow = ComputeCubeMapPCF(light.idxShadowMap, -lightDirVector, distance);

    return result * shadow;
}

vec3 ComputeDirectionalLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    float diff = max(dot(-light.direction, normal), 0.0);
    vec3 diffuse = diff * light.diffuse * albedo;

    vec3 view_dir = normalize(cameraData.position.xyz - fs_in.FragPos);
    vec3 reflectDir = reflect(light.direction, normal);
    vec3 halfwayDir = normalize(-light.direction + view_dir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), uboMaterial.Shininess);
    vec3 specularResult = spec * light.specular * specular;

    vec3 result = diffuse + specularResult + emissive;

    // Get cascade index for the current fragment's view position
	uint cascadeIndex = 0;
	for (uint i = 0; i < CSM_COUNT - 1; ++i)
    {
        vec4 cascadeSplit = QE_Cascade.Splits[light.idxShadowMap];
		if (fs_in.ViewPos.z < cascadeSplit[i])
        {	
			cascadeIndex = i + 1;
		}
	}

    uint viewProjIndex = CSM_COUNT * light.idxShadowMap + cascadeIndex;

	vec4 shadowCoord = (biasMat * QE_CascadeViewProj[viewProjIndex]) * vec4(fs_in.FragPos, 1.0);
    float shadow = ComputeCSMFilterPCF(shadowCoord / shadowCoord.w, light.idxShadowMap, cascadeIndex);

    return result * shadow;
}

vec3 ComputeSpotLight(LightData light, vec3 normal, vec3 albedo, vec3 specular, vec3 emissive)
{
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    float distance = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.direction)); 
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

float ComputeCSMTextureProj(vec4 shadowCoord, vec2 offset, uint lightIdx, uint cascadeIndex)
{
	float shadow = 1.0;
	float bias = 0.005;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
		float dist = texture(QE_DirectionalShadowmaps[lightIdx], vec3(shadowCoord.st + offset, cascadeIndex)).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias)
        {
			shadow = SHADOW_OPACITY;
		}
	}
	return shadow;

}

float ComputeCSMFilterPCF(vec4 sc, uint lightIdx, uint cascadeIndex)
{
	ivec2 texDim = textureSize(QE_DirectionalShadowmaps[lightIdx], 0).xy;
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += ComputeCSMTextureProj(sc, vec2(dx*x, dy*y), lightIdx, cascadeIndex);
			count++;
		}
	}

	return shadowFactor / count;
}

float ComputeCubeMapPCF(uint lightIdx, vec3 lightVec, float distance)
{
    float shadow = 0.0;
    float bias   = 0.15;
    int samples  = 20;
    float z_far = FAR_PLANE;
    float viewDistance = length(cameraData.position.xyz - fs_in.FragPos);
    float diskRadius = (1.0 + (viewDistance / z_far)) / 25.0;

    float currentDepth = length(lightVec);

    for(int i = 0; i < samples; ++i)
    {
        float sampledDist = texture(QE_PointShadowCubemaps[lightIdx], lightVec + sampleOffsetDirections[i] * diskRadius).r;

        if(distance <= sampledDist + EPSILON)
        {
            shadow += SHADOW_OPACITY;
        }
    }

    shadow /= float(samples);

    return shadow;
}