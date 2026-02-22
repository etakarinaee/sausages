#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec3 a_color;

uniform mat4 u_matrix;

out vec3 out_color;

void main() {
    gl_Position = u_matrix * vec4(a_position, 1.0, 1.0);
    out_color = a_color;
} 
