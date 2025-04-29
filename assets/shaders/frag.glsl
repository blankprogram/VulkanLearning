
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

void main() {
    float lighting = max(dot(normalize(fragNormal), normalize(vec3(1,1,1))), 0.1);
    outColor = vec4(vec3(0.6,0.7,0.8) * lighting, 1.0);
}
