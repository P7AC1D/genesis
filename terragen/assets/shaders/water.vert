#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

#define MAX_POINT_LIGHTS 4

struct PointLight {
    vec4 positionAndIntensity;
    vec4 colorAndRadius;
};

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 cameraPosition;
    vec4 sunDirection;
    vec4 sunColor;
    vec4 ambientColor;
    PointLight pointLights[MAX_POINT_LIGHTS];
    ivec4 numPointLights;
    vec4 fogColorAndDensity;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 normalMatrix;
} push;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out vec3 fragViewPos;
layout(location = 4) out float fragDepth;

// Time passed to shader via model matrix w component hack (or we add to UBO)
// For now we'll use world position for wave phase

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    
    // Simple wave animation using world position
    float time = ubo.cameraPosition.w;  // We'll pack time into w component
    float waveHeight = 0.0;
    
    // Multiple wave frequencies for more natural look
    waveHeight += sin(worldPos.x * 0.5 + time * 2.0) * 0.1;
    waveHeight += sin(worldPos.z * 0.7 + time * 1.5) * 0.08;
    waveHeight += sin((worldPos.x + worldPos.z) * 0.3 + time * 2.5) * 0.05;
    
    worldPos.y += waveHeight;
    
    gl_Position = ubo.projection * ubo.view * worldPos;
    
    // Calculate wave normal from derivatives
    float dx = cos(worldPos.x * 0.5 + time * 2.0) * 0.5 * 0.1 +
               cos((worldPos.x + worldPos.z) * 0.3 + time * 2.5) * 0.3 * 0.05;
    float dz = cos(worldPos.z * 0.7 + time * 1.5) * 0.7 * 0.08 +
               cos((worldPos.x + worldPos.z) * 0.3 + time * 2.5) * 0.3 * 0.05;
    
    vec3 waveNormal = normalize(vec3(-dx, 1.0, -dz));
    
    fragPos = worldPos.xyz;
    fragNormal = waveNormal;
    fragTexCoord = inTexCoord;
    fragViewPos = ubo.cameraPosition.xyz;
    fragDepth = gl_Position.z / gl_Position.w;
}
