#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 fragViewPos;
layout(location = 4) in float fragDepth;

layout(location = 0) out vec4 outColor;

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

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 viewDir = normalize(fragViewPos - fragPos);
    vec3 lightDir = normalize(-ubo.sunDirection.xyz);
    
    // Water colors
    vec3 shallowColor = vec3(0.1, 0.5, 0.6);
    vec3 deepColor = vec3(0.0, 0.2, 0.4);
    vec3 foamColor = vec3(0.9, 0.95, 1.0);
    
    // Fresnel effect - more reflective at grazing angles
    float fresnel = pow(1.0 - max(dot(norm, viewDir), 0.0), 3.0);
    fresnel = mix(0.2, 1.0, fresnel);
    
    // Base water color with depth variation (simple approximation)
    vec3 waterColor = mix(shallowColor, deepColor, 0.5);
    
    // Ambient
    vec3 ambient = ubo.ambientColor.rgb * ubo.ambientColor.a * 0.5;
    
    // Diffuse from sun
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.sunColor.rgb * ubo.sunColor.a * 0.3;
    
    // Specular - water is shiny
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * ubo.sunColor.rgb * ubo.sunColor.a * 0.8;
    
    // Sky reflection (simplified - just use a sky color)
    vec3 skyColor = mix(vec3(0.5, 0.7, 1.0), ubo.fogColorAndDensity.rgb, 0.3);
    vec3 reflectDir = reflect(-viewDir, norm);
    float skyFactor = max(reflectDir.y, 0.0);
    vec3 reflection = mix(skyColor * 0.5, skyColor, skyFactor);
    
    // Combine
    vec3 result = waterColor * (ambient + diffuse) + specular;
    result = mix(result, reflection, fresnel * 0.5);
    
    // Fog
    float fogDensity = ubo.fogColorAndDensity.w;
    if (fogDensity > 0.0) {
        float distance = length(fragViewPos - fragPos);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(ubo.fogColorAndDensity.rgb, result, fogFactor);
    }
    
    // Alpha based on fresnel and depth
    float alpha = mix(0.6, 0.9, fresnel);
    
    outColor = vec4(result, alpha);
}
