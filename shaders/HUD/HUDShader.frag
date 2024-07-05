#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// Texture sampler
layout(binding = 1) uniform sampler2D hudTexture;

void main() {
    // Sample the texture
    vec4 texColor = texture(hudTexture, fragTexCoord);
    
    // Apply the tint color (passed from the vertex shader, which is red in this case)
    outColor = texColor * vec4(fragColor, 1.0);
}