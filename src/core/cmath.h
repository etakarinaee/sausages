
#ifndef CMAHTH_H
#define CMAHTH_H

#define RAD2DEG(x) (x * 180.0f / M_PI)
#define DEG2RAD(x) (x * M_PI / 180.0f)

struct vec2 {
    float x;
    float y;
};

struct vec2i {
    int x;
    int y;
};

struct matrix {
    float m[16];
};

struct render_context;

struct vec2 math_vec2_add(struct vec2 a, struct vec2 b);
struct vec2 math_vec2_subtract(struct vec2 a, struct vec2 b);
struct vec2 math_vec2_scale(struct vec2 v, float scalar);

float math_vec2_length(struct vec2 v);
float math_vec2_distance(struct vec2 a, struct vec2 b);
float math_vec2_dot(struct vec2 a, struct vec2 b);
float math_vec2_angle_cos(struct vec2 a, struct vec2 b);

/* Angle in degrees */
float math_vec2_angle(struct vec2 a, struct vec2 b);

struct vec2i math_vec2_to_vec2i(struct vec2 v);
struct vec2 math_vec2i_to_vec2(struct vec2i v); 

void math_matrix_identity(struct matrix *m);
void math_matrix_translate(struct matrix *m, float x, float y, float z);
void math_matrix_scale(struct matrix *m, float x, float y, float z);

/* Angle in degrees */
void math_matrix_rotate_2d(struct matrix* m, float angle);
void math_matrix_mul(struct matrix* out, struct matrix* a, struct matrix* b);
void math_matrix_orthographic(struct matrix* m, float left, float right, float bottom, float top, float near, float far);
void math_matrix_get_orthographic(struct render_context *r, struct matrix* m);



#endif // CMAHTH_H
