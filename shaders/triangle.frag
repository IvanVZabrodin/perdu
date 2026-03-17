// mesh.frag
#version 450

layout(location = 0) in  vec4 in_color;
layout(location = 0) out vec4 out_color;

vec3 face_colors[12] = vec3[](
    vec3(1.0, 0.0, 0.0),  // red
    vec3(0.0, 1.0, 0.0),  // green
    vec3(0.0, 0.0, 1.0),  // blue
    vec3(1.0, 1.0, 0.0),  // yellow
    vec3(1.0, 0.0, 1.0),  // magenta
    vec3(0.0, 1.0, 1.0),  // cyan
    vec3(1.0, 0.5, 0.0),  // orange
    vec3(0.5, 0.0, 1.0),  // purple
    vec3(0.0, 1.0, 0.5),  // mint
    vec3(1.0, 0.0, 0.5),  // pink
    vec3(0.5, 1.0, 0.0),  // lime
    vec3(0.0, 0.5, 1.0)   // sky blue
);


void main() {
	out_color = vec4(face_colors[gl_PrimitiveID % 12], 1.0);
}
