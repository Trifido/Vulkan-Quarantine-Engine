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