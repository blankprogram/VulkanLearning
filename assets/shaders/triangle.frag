
#version 450

layout(location = 1) in vec3 fragColor; // <- receive voxel color

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0); // <- pure voxel color, no texture
}
