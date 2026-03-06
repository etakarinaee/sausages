#version 330 core

layout (location = 0) in vec2 a_position;

uniform mat4 u_model;
uniform mat4 u_proj;

void main() {
    gl_Position = u_proj * u_model * vec4(a_position, 0.0, 1.0);
} 

