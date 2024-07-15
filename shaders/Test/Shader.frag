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


vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {
    vec3 Diffuse = Md * clamp(dot(N, L), 0.0, 1.0);
    vec3 Specular = Ms * vec3(pow(clamp(dot(N, normalize(V + L)), 0.0, 1.0), gamma));

    return (Diffuse + Specular);
}

void main() {
    vec3 viewDir = normalize(ubo.viewerPos - fragPos);
    vec3 normal = normalize(fragNormCoord);

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));

    vec3 ambientLightColor = vec3(1.0, 0.85, 0.7);
    float ambientIntensity = 0.5;  // Aumenta l'intensità della luce ambientale

    // Campiona il colore della texture
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;

    // Inizializza resultColor
    vec3 resultColor;

    if (fragMaterialID == PELLET_MAT) {
        vec3 emissionColor = vec3(0.0f, 0.8f, 0.0f) * 3.0;  // Aumenta l'intensità dell'emissione
        vec3 brdfColor = BRDF(viewDir, normal, lightDir, texColor, vec3(1.0), 32.0);  // Usa BRDF per il calcolo
        resultColor = brdfColor + emissionColor;  // Combina emissione e BRDF
    } else if (fragMaterialID == HUD_MAT) {
        resultColor = texColor * fragColor;
    } else {
        vec3 brdfColor = BRDF(viewDir, normal, lightDir, texColor, vec3(1.0), 32.0);  // Usa BRDF per il calcolo
        vec3 ambient = ambientLightColor * ambientIntensity * texColor;
        resultColor = ambient + brdfColor;  // Combina illuminazione ambientale e BRDF
    }

    outColor = vec4(resultColor, 1.0);
}


// vec3 lambertDiffuse(vec3 albedo, vec3 normal, vec3 lightDir) {
//     float NdotL = max(dot(normal, lightDir), 0.0);
//     return albedo * ubo.lightColor * NdotL;
// }

// vec3 blinnPhongSpecular(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir) {
//     vec3 halfVec = normalize(lightDir + viewDir);
//     float NdotH = max(dot(normal, halfVec), 0.0);
//     float shininess = 32.0;
//     return albedo * ubo.lightColor * pow(NdotH, shininess);
// }

// vec3 orenNayarDiffuse(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
//     float sigma2 = roughness * roughness;
//     float A = 1.0 - 0.5 * (sigma2 / (sigma2 + 0.33));
//     float B = 0.45 * (sigma2 / (sigma2 + 0.09));

//     float NdotL = max(dot(normal, lightDir), 0.0);
//     float NdotV = max(dot(normal, viewDir), 0.0);
//     float thetaI = acos(NdotL);
//     float thetaR = acos(NdotV);

//     float alpha = max(thetaI, thetaR);
//     float beta = min(thetaI, thetaR);

//     float C = sin(alpha) * tan(beta);
//     return albedo * ubo.lightColor * NdotL * (A + B * max(0.0, cos(thetaI - thetaR)) * C);
// }

// // vec3 cookTorranceSpecular(vec3 albedo, vec3 normal, vec3 lightDir, vec3 viewDir, float roughness) {
    
// // }




// void main() {
//     vec3 viewDir = normalize(ubo.viewerPos - fragPos);
//     vec3 lightDir = normalize(ubo.lightDirection);
//     vec3 halfVec = normalize(lightDir + viewDir);
//     vec3 normal = normalize(fragNormCoord);

//     vec3 surfaceColor = texture(texSampler, fragTexCoord).rgb;
//     vec3 ambient = 0.1 * surfaceColor; // Simplified ambient for illustration
//     vec3 resultColor = vec3(0.0);

//     if (fragMaterialID == PELLET_MAT) {
//         vec3 emissionColor = vec3(0.0f, 0.0f, 0.0f);
//         float roughness = 0.001f;
//         vec3 diffuse = orenNayarDiffuse(surfaceColor, normal, lightDir, viewDir, roughness);
//         // vec3 specular = cookTorranceSpecular(surfaceColor, normal, lightDir, viewDir, roughness);
//         // vec3 diffuse = lambertDiffuse(surfaceColor, normal, lightDir);
//         vec3 specular = blinnPhongSpecular(surfaceColor, normal, lightDir, viewDir);

//         resultColor = diffuse + specular + emissionColor;
//     } else if (fragMaterialID == HUD_MAT) {
//         resultColor = surfaceColor * fragColor;
//     } else {
//         vec3 diffuse = lambertDiffuse(surfaceColor, normal, lightDir);
//         vec3 specular = blinnPhongSpecular(surfaceColor, normal, lightDir, viewDir);

//         resultColor = ambient + diffuse + specular;
//     }
   
//     outColor = vec4(resultColor, 1.0);
// }