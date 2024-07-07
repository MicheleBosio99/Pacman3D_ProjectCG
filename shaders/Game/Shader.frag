#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, fragTexCoord) * vec4(fragColor, 1.0);
}

// #version 450

// layout(location = 0) in vec3 fragColor;
// layout(location = 1) in vec2 fragTexCoord;
// layout(location = 2) in flat int materialID;

// layout(location = 0) out vec4 outColor;

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
//     vec3 lightPos;
//     // float time;
// } ubo;

// layout(binding = 1) uniform sampler2D texSampler;



// // BRDF Functions

// // Lambertian for Environment
// vec3 calculateEnvironmentBRDF(vec3 normal, vec3 lightDirection) {
//     normal = normalize(normal);
//     lightDirection = normalize(lightDirection);

//     float NdotL = max(dot(normal, lightDirection), 0.0);

//     vec3 diffuseColor = vec3(0.8, 0.8, 0.8);
//     vec3 ambientColor = vec3(0.2, 0.2, 0.2);

//     return (diffuseColor * NdotL) + ambientColor;
// }

// // Oren-Nayar for Ghosts
// vec3 calculateGhostBRDF(vec3 normal, vec3 lightDirection, vec3 viewDirection) {
//     normal = normalize(normal);
//     lightDirection = normalize(lightDirection);
//     viewDirection = normalize(viewDirection);

//     float roughness = 0.5; 
//     float albedo = 0.8;    

//     float NdotL = max(dot(normal, lightDirection), 0.0);
//     float NdotV = max(dot(normal, viewDirection), 0.0);
//     float angleVN = acos(NdotV);
//     float angleLN = acos(NdotL);

//     float alpha = max(angleVN, angleLN);
//     float beta = min(angleVN, angleLN);

//     float gamma = dot(
//         normalize(viewDirection - normal * NdotV),
//         normalize(lightDirection - normal * NdotL)
//     );

//     float A = 1.0 - 0.5 * (roughness * roughness) / (roughness * roughness + 0.33);
//     float B = 0.45 * (roughness * roughness) / (roughness * roughness + 0.09);
//     float C = sin(alpha) * tan(beta);

//     float orenNayarDiffuse = max(0.0, NdotL) * (A + B * max(0.0, gamma) * C);

//     vec3 lightColor = vec3(0.9, 0.2, 0.2); 

//     return orenNayarDiffuse * albedo * lightColor;
// }

// // Blinn-Phong with Emission for Pellets
// vec3 calculatePelletBRDF(vec3 normal, vec3 lightDirection, vec3 viewDirection) {
//     normal = normalize(normal);
//     lightDirection = normalize(lightDirection);
//     viewDirection = normalize(viewDirection);

//     vec3 diffuseColor = vec3(1.0, 1.0, 0.0);  
//     vec3 specularColor = vec3(1.0, 1.0, 1.0); 
//     float shininess = 128.0;                  

//     vec3 halfVector = normalize(lightDirection + viewDirection);

//     float NdotL = max(dot(normal, lightDirection), 0.0);
//     vec3 diffuse = diffuseColor * NdotL;

//     float NdotH = max(dot(normal, halfVector), 0.0);
//     vec3 specular = specularColor * pow(NdotH, shininess);

//     vec3 emissionColor = vec3(1.0, 1.0, 0.5);   
//     float emissionIntensity = 1.0;

//     // float pulsatingIntensity = emissionIntensity * (0.5 + 0.5 * sin(ubo.time * 2.0));
//     //return diffuse + specular + (emissionColor * pulsatingIntensity); 

//     return diffuse + specular + (emissionColor * emissionIntensity);
// }




// void main() {
//     vec3 normal = normalize(cross(dFdx(fragColor), dFdy(fragColor)));
//     vec3 lightDir = normalize(ubo.lightPos - fragColor);
//     vec3 viewDir = normalize(-fragColor);

//     vec3 brdfResult;

//     // Select appropriate BRDF based on materialID
//     switch (materialID) {
//         case 0:  // Environment
//             brdfResult = calculateEnvironmentBRDF(normal, lightDir);
//             break;
//         case 1:  // Sky
//             brdfResult = calculateGhostBRDF(normal, lightDir, viewDir);
//             break;
//         case 2:  // Pellet
//             brdfResult = calculatePelletBRDF(normal, lightDir, viewDir);
//             break;
//         // Add more cases as needed
//         default:
//             brdfResult = vec3(0.0); // Default to no lighting
//     }
    
//     // Sample the texture
//     vec4 texColor = texture(texSampler, fragTexCoord);

//     // Apply lighting to texture
//     outColor = vec4(texColor.rgb * brdfResult, texColor.a);  
// }