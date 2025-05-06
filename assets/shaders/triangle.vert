#version 450

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inColor;
layout(location=2) in vec4 iModelRow0;
layout(location=3) in vec4 iModelRow1;
layout(location=4) in vec4 iModelRow2;
layout(location=5) in vec4 iModelRow3;

layout(set=0, binding=0) uniform UBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(location=0) out vec3 fragColor;

void main() {
    mat4 instanceModel = mat4(iModelRow0, iModelRow1, iModelRow2, iModelRow3);
    gl_Position = ubo.proj * ubo.view * instanceModel * vec4(inPos, 1.0);
    fragColor   = inColor;
}


