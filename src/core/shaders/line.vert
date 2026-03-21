#version 330 core

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_proj;

uniform vec2 u_start;
uniform vec2 u_end;

void main() {
    vec2 pos = (gl_VertexID == 0) ? u_start : u_end;
    gl_Position = u_proj  * vec4(pos, 0.0, 1.0);
} 

