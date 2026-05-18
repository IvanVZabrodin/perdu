#version 450
layout(location = 0) in vec3 frag_pos;
layout(location = 0) out vec4 out_colour;

void main() {
    vec3 dx     = dFdx(frag_pos);
    vec3 dy     = dFdy(frag_pos);
    vec3 normal = normalize(cross(dx, dy));
    float light = dot(normal, normalize(vec3(1.0, 2.0, 3.0))) * 0.5 + 0.5;
    out_colour  = vec4(vec3(light), 1.0);
}
