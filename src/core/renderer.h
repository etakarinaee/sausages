#ifndef RENDERER_H
#define RENDERER_H

// clang-format off
#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// clang-format on

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include "cmath.h"
#include <stdbool.h>
#include <stddef.h>

#define CORE_RENDERER_QUAD_NO_TEXTURE (-1)

struct color3 {
    float r;
    float g;
    float b;
};

typedef GLint texture_id;
typedef int font_id;

enum {
    QUAD_TYPE_RECT,
    QUAD_TYPE_CIRCLE,
    QUAD_TYPE_TEXTURE,
    QUAD_TYPE_TEXT,
    QUAD_TYPE_MESH,
    QUAD_TYPE_LINE,
};

struct mesh {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    int vertex_count;
    int indices_count;
    struct color3 color; /* little weird to have color here */
};

struct quad_data {
    int type;

    union {
        struct {
            texture_id tex_id;
            struct vec2 min;
            struct vec2 size;
            struct color3 color;
        } text;

        struct {
            texture_id tex_id;
        } texture;

        struct mesh mesh;

        struct {
            struct vec2 start;
            struct vec2 end;
            float width;
        } line;

        struct color3 color;
    } data;

    float rotation;
    struct vec2 scale;
    struct vec2 pos;
};

struct character {
    struct vec2i size;
    struct vec2i bearing;
    uint32_t advance;
    float u0, v0, u1, v1;
};

enum {
    ANCHOR_TOP_LEFT,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_CENTER,
};

struct font {
    struct vec2i char_range; /* from this asci code to another all chars its tmp
                                for more advanced loading */
    struct character
        *chars; /* character data array as big as char_range.y - char_range.x */
    texture_id tex;
    int atlas_width;
    int atlas_height;
    int font_size;
};

struct camera {
    struct vec2 pos;
    float zoom;
};

struct render_context {
    int width;
    int height;
    GLFWwindow *window;
    struct camera camera;

    /* Text */
    FT_Library ft_lib;
    struct font *fonts;
    size_t fonts_count;
    size_t fonts_capacity;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint quad_program;
    GLuint circle_program;
    GLuint tex_program;
    GLuint text_program;
    GLuint mesh_program;
    GLuint line_program;

    struct quad_data *quads;
    size_t quads_count;
    size_t quads_capacity;
};

extern struct render_context render_context;

int renderer_init(struct render_context *r);
void renderer_deinit(const struct render_context *r);

void renderer_push_quad(struct render_context *r, struct quad_data data);
void renderer_push_rect(struct render_context *r, struct vec2 pos,
                        struct vec2 scale, float rotation, struct color3 c,
                        int anchor);
void renderer_push_circle(struct render_context *r, struct vec2 pos,
                          float radius, struct color3 c);
void renderer_push_texture(struct render_context *r, struct vec2 pos,
                           struct vec2 scale, float rotation,
                           texture_id texture, int anchor);
void renderer_push_text(struct render_context *r, struct vec2 pos, float scale,
                        struct color3 text_color, font_id font,
                        const char *text, int anchor);
void renderer_push_mesh(struct render_context *r, struct mesh mesh,
                        struct vec2 pos, struct vec2 scale);
void renderer_push_line(struct render_context *r, struct vec2 start,
                        struct vec2 end, float width);
struct mesh renderer_create_mesh(float *vertices, int vertices_count,
                                 uint32_t *indices, int indices_count,
                                 struct color3 color);
void renderer_update_mesh(struct mesh *mesh, float *vertices, int vertex_count,
                          struct color3 color);
void renderer_draw(struct render_context *r);

texture_id renderer_load_texture(const char *path);
font_id renderer_load_font(struct render_context *r, const char *path,
                           int font_size, struct vec2i char_range);

#endif
