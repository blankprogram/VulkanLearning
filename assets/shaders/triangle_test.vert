
#version 450
// no vertex inputs
layout(location = 0) out vec3 fragColor;
void main() {
    // define 3 clip‑space positions
    const vec2 pts[3] = vec2[3](
        vec2( 0.0,  0.5),
        vec2( 0.5, -0.5),
        vec2(-0.5, -0.5)
    );
    gl_Position = vec4(pts[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec3(1.0, 0.0, 0.0); // solid red
}
