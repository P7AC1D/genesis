#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

#define MAX_POINT_LIGHTS 4

struct PointLight {
    vec4 positionAndIntensity;  // xyz = position, w = intensity
    vec4 colorAndRadius;         // xyz = color, w = radius
};

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec4 sunDirection;
    vec4 sunColor;           // xyz = color, w = intensity
    vec4 ambientColor;       // xyz = color, w = intensity
    PointLight pointLights[MAX_POINT_LIGHTS];
    ivec4 numPointLights;    // x = count
    vec4 fogColorAndDensity; // xyz = color, w = density
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 normalMatrix;
} push;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragViewPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    fragColor = inColor;
    fragNormal = mat3(push.normalMatrix) * inNormal;
    fragPos = worldPos.xyz;
    fragViewPos = ubo.cameraPosition.xyz;
}
