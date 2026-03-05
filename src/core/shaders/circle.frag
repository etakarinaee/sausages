#version 330 core

in vec2 v_local_pos;

uniform vec3 u_color;
uniform float u_radius;

out vec4 fragment_color;

void main() {
    float dist = length(v_local_pos);

    if (dist > 0.5) {
        discard;
    }

    fragment_color = vec4(u_color, 1.0);
}
