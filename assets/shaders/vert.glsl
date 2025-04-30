
#version 450

layout(location = 3) in vec3 inColor;
layout(location = 2) out vec3 fragColor;

void main() {
    gl_Position = ubo.viewProj * vec4(inPos, 1.0);
    fragNormal = inNormal;
    fragUV = inUV;
    fragColor = inColor; // ← pass it
}
