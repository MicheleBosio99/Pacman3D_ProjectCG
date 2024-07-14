#version 450

#define ENVIRONMENT_MAT 0
#define SKY_MAT 1
#define PELLET_MAT 2
#define FROM_FILE_MAT 3
#define HUD_MAT 4

#define PI 3.14159265359f

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormCoord;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) flat in int fragMaterialID;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 modelNorm;
    mat4 view;
    mat4 proj;
    vec3 lightDirection;
    vec3 lightColor;
    vec3 viewerPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

vec3 lambertDiffuse(vec3 albedo, vec3 normal, vec3 lightDir) {
    float NdotL = max(dot(normal, lightDir), 0.0);
    return albedo * ubo.lightColor * NdotL;
}

vec3 blinnPhongSpecular(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir) {
    vec3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVec), 0.0);
    float shininess = 32.0;
    return albedo * ubo.lightColor * pow(NdotH, shininess);
}

vec3 orenNayarDiffuse(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    float sigma2 = roughness * roughness;
    float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
    float B = 0.45 * (sigma2 / (sigma2 + 0.09));

    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    float thetaI = acos(NdotL);
    float thetaR = acos(NdotV);

    float alpha = max(thetaI, thetaR);
    float beta = min(thetaI, thetaR);

    float C = sin(alpha) * tan(beta);
    return albedo * ubo.lightColor * NdotL * (A + B * max(0.0, cos(thetaI - thetaR)) * C);
}

// vec3 cookTorranceSpecular(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    
// }




void main() {
    vec3 viewDir = normalize(ubo.viewerPos - fragPos);
    vec3 lightDir = normalize(ubo.lightDirection);
    vec3 halfVec = normalize(lightDir + viewDir);
    vec3 normal = normalize(fragNormCoord);

    vec3 surfaceColor = texture(texSampler, fragTexCoord).rgb;
    vec3 ambient = 0.1 * surfaceColor; // Simplified ambient for illustration
    vec3 resultColor = vec3(0.0);

    if (fragMaterialID == PELLET_MAT) {
        vec3 emissionColor = vec3(0.0f, 0.0f, 0.0f);
        float roughness = 0.001f;
        vec3 diffuse = orenNayarDiffuse(surfaceColor, normal, lightDir, viewDir, roughness);
        // vec3 specular = cookTorranceSpecular(surfaceColor, normal, lightDir, viewDir, roughness);
        // vec3 diffuse = lambertDiffuse(surfaceColor, normal, lightDir);
        vec3 specular = blinnPhongSpecular(surfaceColor, normal, lightDir, viewDir);

        resultColor = diffuse + specular + emissionColor;
    } else if (fragMaterialID == HUD_MAT) {
        resultColor = surfaceColor * fragColor;
    } else {
        vec3 diffuse = lambertDiffuse(surfaceColor, normal, lightDir);
        vec3 specular = blinnPhongSpecular(surfaceColor, normal, lightDir, viewDir);

        resultColor = ambient + diffuse + specular;
    }
   
    outColor = vec4(resultColor, 1.0);
}




















// Cook-Torrance BRDF;
// vec3 cookTorranceBRDF(vec3 normal, vec3 viewDir, vec3 lightDir, vec3 albedo, float roughness, float metallic) {
//     // Intermediate vectors
//     vec3 halfVector = normalize(viewDir + lightDir); 

//     // Geometry function (Schlick-GGX)
//     float alpha = roughness * roughness;
//     float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
//     float G1o = 1.0 / (dot(normal, viewDir) * (1.0 - k) + k);
//     float G1i = 1.0 / (dot(normal, lightDir) * (1.0 - k) + k);
//     float G = G1o * G1i;

//     // Distribution function (GGX)
//     float D = alpha / (PI * pow(dot(normal, halfVector) * dot(normal, halfVector) * (alpha * alpha - 1.0) + 1.0, 2.0));

//     // Fresnel function (Schlick approximation)
//     vec3 F0 = mix(vec3(0.04), albedo, metallic); // F0 for dielectrics vs. metals
//     float cosTheta = dot(halfVector, viewDir);
//     vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

//     // Cook-Torrance BRDF
//     vec3 specular = F * G * D / (4.0 * dot(normal, viewDir) * dot(normal, lightDir) + 0.001); // Avoid division by zero

//     // Lambertian diffuse (for non-metals)
//     vec3 diffuse = albedo / PI * (1.0 - metallic); 

//     // Final color
//     return (diffuse + specular) * dot(normal, lightDir) * ubo.lightColor;
// }

// vec3 calculateSunlight(vec3 normal, vec3 viewDir) {
//     vec3 sunDir = normalize(ubo.lightDirection);
//     float diff = max(dot(normal, sunDir), 0.0);
//     vec3 diffuse = ubo.lightColor.rgb * diff;
//     vec3 reflectDir = reflect(-sunDir, normal);
//     float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
//     vec3 specular = vec3(0.5) * spec; // Assuming some specular intensity
//     return diffuse + specular;
// }

// void main() {
//     vec3 normal = normalize(fragNormCoord);
//     vec3 viewDir = normalize(-fragPos);
//     vec3 sunlight = calculateSunlight(normal, viewDir);

//     vec3 textureColor = texture(texSampler, fragTexCoord).rgb;
//     vec3 ambient = 0.2 * textureColor;
//     vec3 color = ambient + sunlight * textureColor;

//     if(fragMaterialID == PELLET_MAT) {
//         vec3 luminescence = vec3(0.0f, 0.8, 0.0);
//         color += luminescence;
//     }

//     outColor = vec4(color, 1.0);
// }