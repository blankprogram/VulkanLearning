
#version 450
layout(location=0) in vec3 inPosition;
layout(location=1) in vec2 inUV;
layout(location=0) out vec2 fragUV;

layout(set=0, binding=0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

layout(push_constant) uniform Push {
    mat4 model;
} pc;

void main() {
  fragUV = inUV;
  gl_Position = cam.proj * cam.view * pc.model * vec4(inPosition,1.0);
}
