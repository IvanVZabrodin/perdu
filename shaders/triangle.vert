#version 450
layout(location = 0) in vec4 in_projected;
// layout(location = 1) in vec4 in_worldpos;
layout(location = 0) out vec3 frag_pos;

void main() {
    frag_pos    = vec3(in_projected.xy, in_projected.w);//in_worldpos.xyz;
    gl_Position = vec4(in_projected.xy, in_projected.z, 1.0);
}
