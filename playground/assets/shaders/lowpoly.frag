#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

// Simple directional light
const vec3 lightDir = normalize(vec3(-0.2, -1.0, -0.3));
const vec3 lightColor = vec3(1.0, 0.95, 0.9);
const float ambientStrength = 0.3;

void main() {
    // Ambient
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse (flat shading for low-poly look)
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Combine
    vec3 result = (ambient + diffuse) * fragColor;
    
    // Slight color banding for stylized look
    result = floor(result * 8.0) / 8.0;
    
    outColor = vec4(result, 1.0);
}
