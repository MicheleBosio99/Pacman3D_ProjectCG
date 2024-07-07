#version 450

#define ENVIRONMENT_MAT 0
#define SKY_MAT 1
#define PELLET_MAT 2
#define FROM_FILE_MAT 3
#define HUD_MAT 4

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal; // Assuming you pass normals to the fragment shader

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform Light {
    vec3 sunDirection;
    vec3 sunColor;
} light;

void main() {
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, normalize(light.sunDirection)), 0.0);
    vec3 diffuse = diff * light.sunColor;
    
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 finalColor = texColor.rgb * fragColor * diffuse;
    
    outColor = vec4(finalColor, texColor.a);
}