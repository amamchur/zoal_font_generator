#ifndef FONTTEST_TYPES_HPP
#define FONTTEST_TYPES_HPP

#include <stdint.h>

namespace zoal { namespace text {
    typedef struct {
        uint32_t bitmap_offset;
        uint8_t width;
        uint8_t height;
        uint8_t x_advance;
        int8_t x_offset;
        int8_t y_offset;
    } glyph;

    typedef struct {
        uint16_t first;
        uint16_t second;
        int8_t x_advance;
    } kerning_pair;

    typedef struct {
        uint16_t start;
        uint16_t end;
        uint16_t base;
    } unicode_range;

    typedef struct {
        uint8_t y_advance;
        const uint8_t *bitmap;
        const glyph *glyphs;
        uint16_t glyphs_count;
        const unicode_range *ranges;
        uint16_t ranges_count;
        const kerning_pair *kerning_pairs;
        uint16_t kerning_pairs_count;
    } font;
}}

#endif
