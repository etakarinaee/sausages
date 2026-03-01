
#include "ui.h"
#include "cmath.h"
#include "renderer.h"
#include "collision.h"
#include <GLFW/glfw3.h>

#include <stdio.h>

/* TODO: this is a simple prototype just for testing */
bool ui_button(struct render_context *r, font_id font, const char *text, struct vec2 pos, struct vec2i size) {
    renderer_push_rect(r, pos, math_vec2i_to_vec2(size), 0.0f, (struct color3){1.0f, 0.0f, 0.0f}, ANCHOR_BOTTOM_LEFT); 
    renderer_push_text(r, pos, (float)size.y, (struct color3){1.0f, 1.0f, 1.0f}, font, text, ANCHOR_BOTTOM_LEFT);

    /* change to idk you can decide if press or just down (!= GLFW_RELEASE) */
    if (glfwGetMouseButton(render_context.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        bool clicked = coll_check_point_rect(get_mouse_pos(render_context.window), pos, math_vec2i_to_vec2(size));
        return clicked;
    }

    return false;
}

