
#include "font.h"
#include "freetype/ftimage.h"
#include "renderer.h"

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

static void load_char(FT_Face* face, char c) {
    FT_UInt glyph_index = FT_Get_Char_Index(*face, c);
    FT_Load_Glyph(*face, glyph_index, FT_LOAD_NO_BITMAP);

    if ((*face)->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
        fprintf(stderr, "Glyph is not in outline mode!\n");
        return;
    }

    FT_Outline* outline = &(*face)->glyph->outline;

    for (int i = 0; i < outline->n_points; i++) {
        if (outline->tags[i] & FT_CURVE_TAG_ON) {
            printf("%c: on ", c);
        }
        else {
            printf("%c: off ", c);
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


