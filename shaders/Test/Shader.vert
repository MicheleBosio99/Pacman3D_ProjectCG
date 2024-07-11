#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormCoord;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in int inMaterialID;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 fragNormCoord;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) flat out int fragMaterialID;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 modelNorm;
    mat4 view;
    mat4 proj;
    vec3 lightDirection;
    vec3 lightColor;
    vec3 viewerPos;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    fragPos = (ubo.model * vec4(inPosition, 1.0f)).xyz;
    fragColor = inColor;
    fragNormCoord = inNormCoord;
    fragTexCoord = inTexCoord;
    fragMaterialID = inMaterialID;
}