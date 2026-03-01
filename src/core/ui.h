
#ifndef UI_H
#define UI_H

#include "renderer.h"
#include <stdbool.h>

bool ui_button(struct render_context *r, font_id font, const char *text, struct vec2 pos, struct vec2i size);

#endif // UI_H
