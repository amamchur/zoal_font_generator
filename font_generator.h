#ifndef ZOAL_FONT_GENERATOR_FONT_GENERATOR_H
#define ZOAL_FONT_GENERATOR_FONT_GENERATOR_H

#include "types.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <vector>
#include <fstream>

class font_generator {
public:
    std::string font_path;
    uint8_t font_size{16};
    std::vector<std::string> font_ranges;
    std::vector<zoal::text::glyph> glyphs;
    std::vector<uint8_t> buffer;
    std::vector<zoal::text::kerning_pair> kerning;
    std::string font_name{"roboto_regular_24"};
    std::vector<zoal::text::unicode_range> ranges;
    bool use_progmem{false};
    bool use_kern{false};

    zoal::text::font current_font;

    int rasterize_font();
    int generate_fonts_file();

    void read_kering(FT_Face face);
    void create_bitmap_glyph(FT_GlyphSlot slot);
    void make_range(FT_Face face, FT_ULong range_from, FT_ULong range_to);
    void generate_src(FT_Face face);
    void generate_bitmap(std::fstream &fs);
    void generate_glyphs(std::fstream &fs);
    void generate_ranges(std::fstream &fs);
    void gen_kerning(std::fstream &fs, FT_Face face);
    void generate_font(std::fstream &fs) const;
    bool in_range(FT_ULong value);
};

#endif
