
#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texcoord;

uniform mat4 u_matrix;

out vec2 texcoord;

void main() {
    gl_Position = u_matrix * vec4(a_position, 1.0, 1.0);
    texcoord = a_texcoord;
} 


