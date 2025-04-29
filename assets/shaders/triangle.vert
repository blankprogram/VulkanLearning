
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inColor; // <-- new

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec3 fragColor; // <-- new

layout(set = 0, binding = 0) uniform UBO {
  mat4 view;
  mat4 proj;
} ubo;

layout(push_constant) uniform PushConstant {
  mat4 model;
} pc;

void main() {
  fragUV = inUV;
  fragColor = inColor; // pass color
  gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPos, 1.0);
}
