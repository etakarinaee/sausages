
#ifndef COLLISION_H
#define COLLISION_H

#include "cmath.h"
#include <stdbool.h>
#include <GLFW/glfw3.h>

/* everything in this header is static inline idk if thats how its better but yeah */
/* everything has coll namespace for collision */
static inline bool coll_check_point_circle(struct vec2 p, struct vec2 center, float radius) {
    float dist = math_vec2_distance(p, center);
    if (dist >= radius) return false;
    return true;
}

static inline bool coll_check_point_rect(struct vec2 p, struct vec2 pos, struct vec2 size) {
    return ( 
        p.x > pos.x && p.x < pos.x + size.x && 
        p.y > pos.y && p.y < pos.y + size.y
    );
}

/* probaly should be in collison */
static inline struct vec2 get_mouse_pos(GLFWwindow *window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height); /* bad to get window size every time */

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    struct vec2 pos = {
        .x = (float)x - width * 0.5f,
        .y = (height - (float)y) - height * 0.5f,
    };

    return pos;
}

#endif // COLLISION_H
