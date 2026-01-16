#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 projection;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 ambientColor;
} ubo;

void main() {
    // Normalize inputs
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-ubo.lightDirection.xyz);
    
    // Ambient
    vec3 ambient = ubo.ambientColor.rgb * ubo.ambientColor.a;
    
    // Diffuse (flat shading for low-poly look)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor.rgb * ubo.lightColor.a;
    
    // Combine lighting with vertex color
    vec3 result = (ambient + diffuse) * fragColor;
    
    // Slight color banding for stylized low-poly look
    result = floor(result * 8.0) / 8.0;
    
    outColor = vec4(result, 1.0);
}
