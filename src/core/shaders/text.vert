
#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_proj;

out vec2 texcoord;

void main() {
    gl_Position = u_proj * u_model * vec4(a_position, 0.0, 1.0);
    texcoord = a_texcoord;
} 


