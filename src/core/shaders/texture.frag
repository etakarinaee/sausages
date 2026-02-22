
#version 330 core

in vec2 texcoord;

out vec4 fragment_color;

uniform sampler2D u_texture;

void main() {
    vec4 tex_color = texture(u_texture, texcoord);
    fragment_color = vec4(tex_color.rgb * tex_color.a, tex_color.a);
    
    if (fragment_color.a <= 0.0) discard;
}
