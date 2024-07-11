#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, inTexCoord); 
    // texColor.a = 1.0; // Adjust to make it transparent as needed;
    outColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color;
}