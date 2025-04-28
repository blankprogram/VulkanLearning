
#version 450

// add this so it matches the VS’s out variable:
layout(location=0) in vec2 fragUV;

layout(location=0) out vec4 outColor;

void main() {
    outColor = vec4(1.0); // solid white
}
