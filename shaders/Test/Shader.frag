#version 450

#define ENVIRONMENT_MAT 0
#define SKY_MAT 1
#define PELLET_MAT 2
#define POWER_PELLET_MAT 3
#define FROM_FILE_MAT 4
#define HUD_MAT 5

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
} ubo;

layout(binding = 1) uniform GlobalUniformBufferObject {
    vec3 viewerPos;
    vec3 ambientLightDirection;
    vec3 ambientLightColor;
    int pointLightCount;
    vec3 pointLightPos[12];
    vec4 pointLightColors[12];
    int ghostLightCount;
    vec3 ghostsLightsPositions[4];
    vec4 ghostsLightsColors[4];
} gubo;

layout(binding = 2) uniform sampler2D texSampler;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

vec3 CookTorranceBRDF(vec3 V, vec3 N, vec3 L, vec3 F0, float roughness, vec3 albedo) {
    vec3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

    vec3 nominator = NDF * G * F;
    float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f);
    vec3 specular = nominator / max(denominator, 0.001f);

    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 0.5f;

    float NdotL = max(dot(N, L), 0.0f);
    return (kD * albedo / PI + specular) * NdotL;
}

void main() {
    vec3 viewDir = normalize(gubo.viewerPos - fragPos);
    vec3 normal = normalize(fragNormCoord);

    vec3 lightDir = normalize(gubo.ambientLightDirection);

    vec3 ambientLightColor = vec3(1.0f, 0.85f, 0.7f);
    float ambientIntensity = 1.0f;  // Higher ambient light intensity;
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;

    vec3 resultColor = vec3(0.0f);
    vec3 F0 = vec3(0.3f); // Higher F0 for more specular reflection;
    float roughness = 0.4f; // Less roughness, more lucid effect;

    if (fragMaterialID == PELLET_MAT) {
        vec3 emissionColor = vec3(0.0f, 0.6f, 0.1f);
        vec3 brdfColor = CookTorranceBRDF(viewDir, normal, lightDir, F0, roughness, texColor);
        resultColor = brdfColor + emissionColor;
    } 
    else if (fragMaterialID == POWER_PELLET_MAT) {
        vec3 emissionColor = vec3(0.1f, 0.0f, 0.6f);
        vec3 brdfColor = CookTorranceBRDF(viewDir, normal, lightDir, F0, roughness, texColor);
        resultColor = brdfColor + normal * 0.5 + 0.5;
    } else if (fragMaterialID == HUD_MAT) {
        resultColor = texColor * fragColor;
    } else {
        vec3 brdfColor = CookTorranceBRDF(viewDir, normal, lightDir, F0, roughness, texColor);
        vec3 ambient = ambientLightColor * ambientIntensity * texColor;
        resultColor = ambient + brdfColor;
    }

    outColor = vec4(resultColor, 1.0);
}