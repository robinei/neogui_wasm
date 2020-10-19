#include "neogui.h"
#include <stdio.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#define FONT_SIZE 36

EXPORT void test_text(const char *font_data, uint32_t font_data_size) {
    hb_blob_t *blob = hb_blob_create(font_data, font_data_size, HB_MEMORY_MODE_READONLY, NULL, NULL);
    hb_face_t *face = hb_face_create(blob, 0);
    hb_blob_destroy(blob); // face keeps reference
    
    hb_font_t *font = hb_font_create(face);
    hb_ot_font_set_funcs(font);
    hb_font_set_scale(font, FONT_SIZE*64, FONT_SIZE*64);

    hb_buffer_t *buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, "iimm", -1, 0, -1);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(font, buf, NULL, 0);

    unsigned int glyph_count = hb_buffer_get_length(buf);
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, NULL);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, NULL);

    double cursor_x = 0;
    double cursor_y = 0;
    for (unsigned int i = 0; i < glyph_count; ++i) {
        hb_codepoint_t glyphid = glyph_info[i].codepoint;
        printf("%d (%.1f, %.1f)\n", glyphid, cursor_x + glyph_pos[i].x_offset / 64., cursor_y + glyph_pos[i].y_offset / 64.);
        cursor_x += glyph_pos[i].x_advance / 64.;
        cursor_y += glyph_pos[i].y_advance / 64.;
    }

    hb_buffer_destroy(buf);
    hb_font_destroy(font);
    hb_face_destroy(face);
}

