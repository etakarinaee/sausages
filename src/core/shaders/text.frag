#version 330 core

in vec2 texcoord;
out vec4 fragment_color;

uniform sampler2D u_texture;
uniform vec3 u_text_color;

uniform vec2 u_glyph_min;   // (u0, v0)
uniform vec2 u_glyph_size;  // (u1-u0, v1-v0)

void main() {
    vec2 atlasUV = u_glyph_min + vec2(texcoord.x, 1.0 - texcoord.y) * u_glyph_size;
    float alpha = texture(u_texture, atlasUV).r;
    fragment_color = vec4(u_text_color, alpha);    
}
