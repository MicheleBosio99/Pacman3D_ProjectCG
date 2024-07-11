#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormCoord;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in int inMaterialID;

layout(location = 0) out vec2 fragTexCoord;

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
    // gl_Position = ubo.proj * vec4(inPosition, 1.0);
    // fragTexCoord = inTexCoord;
    // Define the positions of a square in normalized device coordinates (NDC)
    vec2 positions[4] = vec2[](
        vec2(-0.5, -0.5),
        vec2(0.5, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    );

    // Define the vertex position
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragTexCoord = positions[gl_VertexIndex];
}