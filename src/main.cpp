/*
 * Game splash screen demo for the Game Boy Advance, built with Butano.
 *
 * It renders a small selection of UI elements to learn the basics of GBA
 * programming: a sprite loaded from a bitmap, a full-screen bitmap background
 * used as a canvas to draw the red fill, white section outlines and the two
 * white button boxes, plus multi-colored text at two sizes.
 */

#include "bn_core.h"
#include "bn_color.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_bpp_mode.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_text_generator.h"
#include "bn_sp_direct_bitmap_bg_ptr.h"
#include "bn_sp_direct_bitmap_bg_painter.h"

#include "bn_sprite_items_b.h"
#include "bn_sprite_items_lock.h"

#include "common_variable_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

namespace
{
    // The common fonts draw their glyphs mostly with palette color 14 (white)
    // outlined with color 12 (black). To get yellow text we reuse that palette
    // but swap color 14 for pure yellow. Colors are the 5-bit-per-channel GBA
    // equivalent of the font bitmap's own palette.
    constexpr bn::color yellow_font_colors[16] = {
        bn::color(0, 31, 0),    //  0 - transparent (unused by glyph pixels)
        bn::color(6, 8, 11),    //  1
        bn::color(13, 7, 11),   //  2
        bn::color(22, 6, 8),    //  3
        bn::color(28, 7, 7),    //  4
        bn::color(12, 12, 12),  //  5
        bn::color(9, 12, 16),   //  6
        bn::color(30, 12, 8),   //  7
        bn::color(12, 16, 23),  //  8
        bn::color(12, 17, 23),  //  9
        bn::color(31, 18, 10),  // 10
        bn::color(16, 23, 20),  // 11
        bn::color(0, 0, 0),     // 12 - glyph outline (black)
        bn::color(23, 24, 24),  // 13 - glyph edge (light grey)
        bn::color(31, 31, 0),   // 14 - glyph body -> yellow
        bn::color(1, 0, 0)      // 15
    };

    constexpr bn::sprite_palette_item yellow_font_palette(yellow_font_colors, bn::bpp_mode::BPP_4);

    // Black text for the white-filled buttons. The glyph body (index 14) is
    // black, and every other glyph color (the outline / anti-aliased "shadow"
    // pixels) is set to white so it blends into the white button and leaves
    // clean, readable black text with no shadow.
    constexpr bn::color black_font_colors[16] = {
        bn::color(0, 31, 0),    //  0 - transparent (unused by glyph pixels)
        bn::color(31, 31, 31),  //  1
        bn::color(31, 31, 31),  //  2
        bn::color(31, 31, 31),  //  3
        bn::color(31, 31, 31),  //  4
        bn::color(31, 31, 31),  //  5
        bn::color(31, 31, 31),  //  6
        bn::color(31, 31, 31),  //  7
        bn::color(31, 31, 31),  //  8
        bn::color(31, 31, 31),  //  9
        bn::color(31, 31, 31),  // 10
        bn::color(31, 31, 31),  // 11
        bn::color(31, 31, 31),  // 12 - glyph outline -> white (hidden)
        bn::color(31, 31, 31),  // 13 - glyph edge -> white (hidden)
        bn::color(0, 0, 0),     // 14 - glyph body -> black
        bn::color(31, 31, 31)   // 15
    };

    constexpr bn::sprite_palette_item black_font_palette(black_font_colors, bn::bpp_mode::BPP_4);

    // The maroon red of the lock bitmap (#800000) as a GBA color.
    constexpr bn::color background_red(16, 0, 0);

    // Horizontal center of each of the two info boxes.
    constexpr bn::fixed left_box_x = -60;
    constexpr bn::fixed right_box_x = 60;

    // Countdown values, shown as static "DD:HH:MM:SS" strings for now.
    constexpr const char* left_countdown = "02:23:57:37";
    constexpr const char* right_countdown = "06:23:57:37";
}

int main()
{
    bn::core::init();

    // Full-screen bitmap background used as a drawing canvas. It is filled with
    // the same red as the lock bitmap (so the lock blends into the corner) and
    // then used to draw the white outlines around each section. Everything else
    // (text, lock) is made of sprites, which always render on top of the
    // background.
    bn::sp_direct_bitmap_bg_ptr canvas = bn::sp_direct_bitmap_bg_ptr::create();
    bn::sp_direct_bitmap_bg_painter painter(canvas);
    painter.fill(background_red);

    // Coordinates below are given in the same centered space used for sprites
    // (origin at the screen center, +x right, +y down) and converted to the
    // bitmap's top-left origin.
    const bn::color box_color(31, 31, 31);

    // Draws a white rectangle outline around a section.
    auto draw_box = [&painter, box_color](int left, int top, int right, int bottom)
    {
        int bl = left + 120, bt = top + 80, br = right + 120, bb = bottom + 80;
        painter.horizontal_line(bl, br, bt, box_color);
        painter.horizontal_line(bl, br, bb, box_color);
        painter.vertical_line(bl, bt, bb, box_color);
        painter.vertical_line(br, bt, bb, box_color);
    };

    // Draws a solid white filled button box.
    auto fill_box = [&painter, box_color](int left, int top, int right, int bottom)
    {
        painter.rectangle(left + 120, top + 80, right + 120, bottom + 80, box_color);
    };

    draw_box(-118, -46, -2, 8);   // Left "Time Left" countdown
    draw_box(2, -46, 118, 8);     // Right "Time Left" countdown
    draw_box(-95, 14, 95, 50);    // Bitcoin section
    fill_box(-100, 56, -20, 76);  // Check Clock button (white fill, black text)
    fill_box(20, 56, 100, 76);    // Decrypt button (white fill, black text)

    // Lock bitmap near the top-left corner (a 32x32 sprite), nudged 15 pixels
    // to the right of the corner.
    bn::sprite_ptr lock_sprite = bn::sprite_items::lock.create_sprite(-89, -64);

    // Two font sizes: 8x8 for labels/values, 8x16 for the prominent lines.
    bn::sprite_text_generator small_text(common::variable_8x8_sprite_font);
    bn::sprite_text_generator big_text(common::variable_8x16_sprite_font);
    small_text.set_center_alignment();
    big_text.set_center_alignment();

    // The generators' default palette renders white; keep a copy so we can
    // switch back after drawing yellow text.
    const bn::sprite_palette_item white_small_palette = small_text.palette_item();

    // --- Static text (generated once, kept alive for the whole program) ------
    bn::vector<bn::sprite_ptr, 64> static_sprites;

    big_text.generate(25, -72, "Ooops, your files", static_sprites);
    big_text.generate(25, -60, "have been encrypted!", static_sprites);

    // Yellow section titles.
    small_text.set_palette_item(yellow_font_palette);
    small_text.generate(left_box_x, -38, "Payment will raise on", static_sprites);
    small_text.generate(right_box_x, -38, "Files will be lost on", static_sprites);

    // White timestamps.
    small_text.set_palette_item(white_small_palette);
    small_text.generate(left_box_x, -27, "6/15/2026 8:47:55", static_sprites);
    small_text.generate(right_box_x, -27, "6/21/2026 8:47:55", static_sprites);

    // Yellow "Time Left" labels.
    small_text.set_palette_item(yellow_font_palette);
    small_text.generate(left_box_x, -15, "Time Left", static_sprites);
    small_text.generate(right_box_x, -15, "Time Left", static_sprites);

    // Countdown values in the larger white font.
    big_text.generate(left_box_x, -1, left_countdown, static_sprites);
    big_text.generate(right_box_x, -1, right_countdown, static_sprites);

    // Alphanumeric section: B icon on the left, then the yellow label and white
    // value left-aligned to its right.
    bn::sprite_ptr b_sprite = bn::sprite_items::b.create_sprite(-77, 32);
    small_text.set_left_alignment();
    small_text.generate(-55, 26, "Send $300 of bitcoin to:", static_sprites);
    small_text.set_palette_item(white_small_palette);
    small_text.generate(-55, 38, "12t9DYPgweZ9NgM5b7SMw", static_sprites);
    small_text.set_center_alignment();

    // Buttons: black text centered on the white-filled boxes drawn above.
    small_text.set_palette_item(black_font_palette);
    small_text.generate(left_box_x, 66, "Check Payment", static_sprites);
    small_text.generate(right_box_x, 66, "Decrypt", static_sprites);

    while(true)
    {
        bn::core::update();
    }
}
