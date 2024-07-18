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

vec3 point_light_dir(vec3 pos, int i) { return normalize(gubo.pointLightPos[i] - pos); }

vec3 point_light_color(vec3 pos, int i) {
	vec3 lightColor = gubo.pointLightColors[i].rgb;
	float g = gubo.pointLightColors[i].a;
	float beta = 2.0;
	vec3 lightPos = gubo.pointLightPos[i];
	float attenuation = pow((g / length(lightPos - pos)), beta);
	return lightColor * attenuation;
}


vec3 BRDF(vec3 V, vec3 N, vec3 L, vec3 Md, vec3 Ms, float gamma) {
    vec3 Diffuse = Md * clamp(dot(N, L), 0.0, 1.0);
    vec3 Specular = Ms * vec3(pow(clamp(dot(N, normalize(V + L)), 0.0, 1.0), gamma));

    return (Diffuse + Specular);
}

vec3 BRDF1(vec3 Albedo, vec3 Norm, vec3 EyeDir, vec3 LD) {
// Compute the BRDF, with a given color <Albedo>, in a given position characterized bu a given normal vector <Norm>,
// for a light direct according to <LD>, and viewed from a direction <EyeDir>
	vec3 Diffuse;
	vec3 Specular;
	Diffuse = Albedo * max(dot(Norm, LD),0.0f);
	Specular = vec3(pow(max(dot(EyeDir, -reflect(LD, Norm)),0.0f), 160.0f));
	
	return Diffuse + Specular;
}

void main() {
    vec3 viewDir = normalize(gubo.viewerPos - fragPos);
    vec3 normal = normalize(fragNormCoord);

    vec3 lightDir = normalize(vec3(0.0f, 1.0f, 0.0f));

    vec3 ambientLightColor = vec3(1.0f, 0.85f, 0.7f);
    float ambientIntensity = 0.1f;  // Ambient light intensity;

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

        // for (int i = 0; i < gubo.pointLightCount; i ++) {
        //     resultColor +=
        //         BRDF1(texColor, normal, viewDir, point_light_dir(fragPos, i)) * point_light_color(fragPos, i);
        // }
    }

    outColor = vec4(resultColor, 1.0);
}