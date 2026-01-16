#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 fragViewPos;

layout(location = 0) out vec4 outColor;

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

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightPos = light.positionAndIntensity.xyz;
    float intensity = light.positionAndIntensity.w;
    vec3 lightColor = light.colorAndRadius.xyz;
    float radius = light.colorAndRadius.w;
    
    vec3 lightDir = lightPos - fragPos;
    float distance = length(lightDir);
    
    // Early out if outside light radius
    if (distance > radius) return vec3(0.0);
    
    lightDir = normalize(lightDir);
    
    // Attenuation
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    attenuation *= intensity;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * attenuation;
    
    // Simple specular for point lights
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    vec3 specular = spec * lightColor * attenuation * 0.3;
    
    return diffuse + specular;
}

void main() {
    // Normalize inputs
    vec3 norm = normalize(fragNormal);
    vec3 viewDir = normalize(fragViewPos - fragPos);
    vec3 lightDir = normalize(-ubo.sunDirection.xyz);
    
    // Ambient
    vec3 ambient = ubo.ambientColor.rgb * ubo.ambientColor.a;
    
    // Directional light (sun) - diffuse
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 sunDiffuse = diff * ubo.sunColor.rgb * ubo.sunColor.a;
    
    // Directional light - specular (subtle for low-poly)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 sunSpecular = spec * ubo.sunColor.rgb * ubo.sunColor.a * 0.2;
    
    // Point lights
    vec3 pointLighting = vec3(0.0);
    for (int i = 0; i < ubo.numPointLights.x && i < MAX_POINT_LIGHTS; i++) {
        pointLighting += calculatePointLight(ubo.pointLights[i], norm, fragPos, viewDir);
    }
    
    // Combine lighting with vertex color
    vec3 result = (ambient + sunDiffuse + sunSpecular + pointLighting) * fragColor;
    
    // Fog
    float fogDensity = ubo.fogColorAndDensity.w;
    if (fogDensity > 0.0) {
        float distance = length(fragViewPos - fragPos);
        float fogFactor = exp(-fogDensity * distance);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        result = mix(ubo.fogColorAndDensity.rgb, result, fogFactor);
    }
    
    // Slight color banding for stylized low-poly look
    result = floor(result * 10.0) / 10.0;
    
    outColor = vec4(result, 1.0);
}
