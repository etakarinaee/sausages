
#version 330 core

in vec3 out_color;
in vec2 texcoord;

out vec4 fragment_color;

uniform sampler2D u_texture;

void main() {
    vec4 tex_color = texture(u_texture, texcoord);
    fragment_color = tex_color;
    
    if (fragment_color.a <= 0.0) discard;
}
