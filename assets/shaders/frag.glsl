

#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    // No lighting applied — use voxel color directly
    outColor = vec4(fragColor, 1.0);
}

