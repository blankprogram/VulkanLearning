
#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, -1.0, -0.5));
    float diffuse = max(dot(fragNormal, lightDir), 0.0);
    vec3 shadedColor = fragColor * diffuse;
    outColor = vec4(shadedColor, 1.0);
}

