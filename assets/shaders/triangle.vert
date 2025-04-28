
#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inColor;
layout(location=2) in vec2 inUV;

layout(location=0) out vec3 fragColor;
layout(location=2) out vec2 fragUV;

layout(set=0,binding=0) uniform UBO {
    mat4 model, view, proj;
} ubo;

void main() {
    fragColor = inColor;  // if you really want tinting
    fragUV    = inUV;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition,1);
}
