

#version 450

// we no longer need normals or UVs here
layout(location = 0) out vec4 outColor;

void main() {
    // flat, constant color for every fragment:
    outColor = vec4(0.6, 0.7, 0.8, 1.0);
}

