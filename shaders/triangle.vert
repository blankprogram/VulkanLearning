#version 450

// binding=0, set=0 matches how you created your descriptor set layout
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// If you’re passing vertex position & color via a VkVertexBuffer,
// you can use input attributes instead of gl_VertexIndex:
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

void main() {
    // apply the full MVP transform
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 0.0, 1.0);
    fragColor  = inColor;
}
