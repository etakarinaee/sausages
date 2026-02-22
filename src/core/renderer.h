#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

struct vec2 {
    float x;
    float y;
};

struct color3 {
    float r;
    float g;
    float b;
};

struct matrix {
    float m[16];
};

typedef GLint texture_id;

struct quad_data {
    texture_id tex;
    float scale;
    float rotation;
    struct vec2 pos;
    struct color3 color;
};

struct render_context {
    int width;
    int height;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    GLuint quad_program;
    GLuint tex_program;

    struct quad_data *quads;
    int quads_count;
    int quads_capacity;
};

extern struct render_context ctx;
extern GLFWwindow* window;

int renderer_init(struct render_context *ctx);

void renderer_deinit(struct render_context *ctx);

void renderer_push_quad(struct render_context *ctx, struct vec2 pos, float scale, float rotation, struct color3 c);

void renderer_draw(struct render_context *ctx);

texture_id renderer_load_texture(const char* path);

/* Math */
void math_matrix_identity(struct matrix *m);

void math_matrix_translate(struct matrix *m, float x, float y, float z); /* Maybe vec3 idk this is jus temp */
void math_matrix_scale(struct matrix *m, float x, float y, float z);

#endif
