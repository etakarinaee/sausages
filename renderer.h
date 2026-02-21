
#ifndef REDNERER_H
#define REDNERER_H

#include <glad/glad.h>

struct vec2 {
    float x;
    float y;
};

struct matrix {
    float m[16];
};

struct quad_data {
    struct vec2 pos;
    float scale;
    float rotation;
};

struct render_context {
    int width;
    int height;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint program;

    struct quad_data* quads;
    int quads_count;
    int quads_capacity;
};

int renderer_init(struct render_context* ctx);
void renderer_deinit(struct render_context* ctx);
void renderer_push_quad(struct render_context* ctx, struct vec2, float scale, float rotation);
void renderer_draw(struct render_context* ctx);

/* Math */
void math_matrix_indentity(struct matrix* m);
void math_matrix_translate(struct matrix* m, float x, float y, float z); /* Maybe vec3 idk this is jus temp */
void math_matrix_scale(struct matrix* m, float x, float y, float z);

#endif 
