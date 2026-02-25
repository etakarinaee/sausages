
#include "font.h"
#include "freetype/ftimage.h"
#include "renderer.h"
#include "cmath.h"

#include <stdlib.h>

int font_init(struct render_context *r) {
    if (FT_Init_FreeType(&r->ft_lib)) {
        fprintf(stderr, "failed to init freetype\n");
        return 1;
    }

    return 0;
}

void font_deinit(const struct render_context *r) {
    FT_Done_FreeType(r->ft_lib);
}

// control points: p1
static struct vec2 quadratic_bezier_derivitive(struct vec2 p0, struct vec2 p1, struct vec2 p2, float t) {
    struct vec2 term_1 = math_vec2_scale(math_vec2_subtract(p2, math_vec2_scale(p1, 2)), 2.0f * t);
    struct vec2 term_2 = math_vec2_scale(math_vec2_subtract(p1, p0), 2.0f);
    return math_vec2_add(term_1, term_2);
}

// control points: p1 p2
static struct vec2 cubic_bezer_derivitive(struct vec2 p0, struct vec2 p1, struct vec2 p2, struct vec2 p3, float t) {
    struct vec2 term_1 = math_vec2_scale(math_vec2_add(math_vec2_subtract(p3, math_vec2_scale(p2, 3)),
                                                       math_vec2_subtract(math_vec2_scale(p1, 3.0f), p0)), 3.0f * t * t);
    struct vec2 term_2 = math_vec2_scale(math_vec2_add(math_vec2_subtract(p2, math_vec2_scale(p1, 2.0f)), p0), 6.0f * t);
    struct vec2 term_3 = math_vec2_scale(math_vec2_subtract(p1, p0), 3.0);
    return math_vec2_add(term_1, math_vec2_add(term_2, term_3));
}

static void load_char(FT_Face* face, char c) {
    FT_UInt glyph_index = FT_Get_Char_Index(*face, c);
    FT_Load_Glyph(*face, glyph_index, FT_LOAD_NO_BITMAP);

    if ((*face)->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        fprintf(stderr, "Glyph is not in outline mode!\n");
        return;
    }

    FT_Outline* outline = &(*face)->glyph->outline;

    for (int i = 0; i < outline->n_points; i++) {
        if (outline->tags[i] & 1) {
            printf("%c: on ", c);
        }
        else {
            if (outline->tags[i] & 3) {
                printf("%c: off cubic ", c);
            }
            else {
                printf("%c: off quadratic ", c);
            }
        }

        printf("%ld %ld\n", outline->points[i].x, outline->points[i].y);
    }
}

uint8_t* font_get_atlas(const char* path, int *width, int *height) {
    if (!path) return NULL;

    const int chars_count = '~' - '0';
    const int char_len = 64;

    *width = char_len * chars_count;
    *height = char_len * chars_count;

    uint8_t* data = calloc(*width * *height * 4, sizeof(uint8_t));
    if (!data) {
        fprintf(stderr, "out of memory loading font: %s\n", path);
        return NULL;
    }

    FT_Face face;

    FT_Error error = FT_New_Face(ctx.ft_lib, path, 0, &face);
    if (error == FT_Err_Unknown_File_Format) {
        fprintf(stderr, "unkown format: %s\n", path);
        return NULL;
    }
    else if (error) {
        fprintf(stderr, "faced unkown error when loading: %s\n", path);
        return NULL;
    }

    // Set Pixel Sizes better probaly
    //error = FT_Set_Char_Size(face, 0, 64 * 64, 0, 0);
    error = FT_Set_Pixel_Sizes(face, 0, 64);

    for (char c = '0'; c < '~'; c++) {
        load_char(&face, c);
    }

    return data;
}


