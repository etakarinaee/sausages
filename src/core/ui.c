
#include "ui.h"
#include "cmath.h"
#include "renderer.h"

bool ui_button(struct render_context *r, font_id font, const char *text, struct vec2 pos, struct vec2i size) {
    renderer_push_rect(r, pos, math_vec2i_to_vec2(size), 0.0f, (struct color3){1.0f, 0.0f, 0.0f}, ANCHOR_BOTTOM_LEFT); 
    renderer_push_text(r, pos, (float)size.y, (struct color3){1.0f, 1.0f, 1.0f}, font, text, ANCHOR_BOTTOM_LEFT);

    return false;
}

