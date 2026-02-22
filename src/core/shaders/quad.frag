#version 330 core

out vec4 fragment_color;

uniform vec3 u_color;

void main() {
    fragment_color = vec4(u_color, 1.0);
}
