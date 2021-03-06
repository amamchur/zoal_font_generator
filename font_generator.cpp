#include "font_generator.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <ft2build.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include FT_FREETYPE_H

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

bool font_generator::in_range(FT_ULong value) {
    for (auto &r : ranges) {
        if (r.start <= value && value <= r.end) {
            return true;
        }
    }
    return false;
}

int font_generator::rasterize_font() {
    return 0;
}

int font_generator::generate_fonts_file() {
    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        return -1;
    }

    FT_Face face;
    error = FT_New_Face(library, font_path.c_str(), 0, &face);
    if (error) {
        FT_Done_FreeType(library);
        return -1;
    }

    error = FT_Set_Pixel_Sizes(face, 0, font_size);
    if (error) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return -1;
    }

    for (auto &rng : font_ranges) {
        std::vector<std::string> strings;
        boost::split(strings, rng, boost::is_any_of("-"));

        if (strings.size() != 2) {
            continue;
        }

        auto start = std::stoul(strings[0], nullptr, 16);
        auto end = std::stoul(strings[1], nullptr, 16);
        make_range(face, start, end);
    }

    current_font.y_advance = font_size;
    current_font.ranges = ranges.data();
    current_font.ranges_count = ranges.size();
    current_font.bitmap = buffer.data();
    current_font.glyphs = glyphs.data();
    current_font.glyphs_count = glyphs.size();
    current_font.kerning_pairs = kerning.data();
    current_font.kerning_pairs_count = kerning.size();

    generate_src(face);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

void font_generator::make_range(FT_Face face, FT_ULong range_from, FT_ULong range_to) {
    zoal::text::unicode_range r;
    r.start = range_from;
    r.end = range_to;
    r.base = glyphs.size();
    ranges.push_back(r);

    FT_GlyphSlot slot = face->glyph;
    for (FT_ULong code = range_from; code <= range_to; code++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, code);
        FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (error) {
            continue;
        }

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
        if (error) {
            continue;
        }
        create_bitmap_glyph(slot);
    }
}

void font_generator::generate_src(FT_Face face) {
    std::fstream fs;
    std::string cpp_file = font_name;
    cpp_file = "../" + cpp_file + ".cpp";

    fs.open(cpp_file, std::fstream::out);
    fs << "#include \"" << font_name << ".hpp\"" << std::endl;
    if (use_progmem) {
        fs << "#include <avr/pgmspace.h>" << std::endl;
    }

    fs << std::endl;

    generate_bitmap(fs);
    generate_glyphs(fs);
    generate_ranges(fs);
    if (use_kern) {
        gen_kerning(fs, face);
    }
    generate_font(fs);

    fs.close();

    std::string def_name = font_name;
    std::string str = "Hello World";
    std::transform(def_name.begin(), def_name.end(), def_name.begin(), ::toupper);

    std::string hpp_file = font_name;
    hpp_file = "../" + hpp_file + ".hpp";
    fs.open(hpp_file, std::fstream::out);
    fs << "#ifndef " << def_name << std::endl;
    fs << "#define " << def_name << std::endl;
    fs << "#include <zoal/text/types.hpp>" << std::endl;
    fs << "extern const zoal::text::font " << font_name << ";" << std::endl;
    fs << "#endif" << std::endl;
    fs.close();
}

void font_generator::create_bitmap_glyph(FT_GlyphSlot slot) {
    FT_Bitmap &bitmap = slot->bitmap;
    zoal::text::glyph g{};
    g.bitmap_offset = buffer.size();
    g.x_offset = (int8_t) slot->bitmap_left;
    g.y_offset = (int8_t) (-slot->bitmap_top);
    g.width = bitmap.width;
    g.height = bitmap.rows;
    g.x_advance = slot->advance.x >> 6;
    glyphs.push_back(g);

    auto bytes = ((bitmap.width + 7) >> 3);
#if 0
    uint8_t glyph_row[32];
    char str[256];
    for (int y = 0; y < bitmap.rows; y++) {
        uint8_t *row = bitmap.buffer + bitmap.pitch * y;
        memset(str, 0, sizeof(str));
        memset(glyph_row, 0, sizeof(glyph_row));
        int index = 0;
        for (int k = 0; k < bitmap.width; k++) {
            uint8_t pixel = row[k];
            int bt = k / 8;
            int offset = 7 - k % 8;
            glyph_row[bt] |= (pixel < 100 ? 0 : 1) << offset;

            str[index++] = pixel < 50 ? ' ' : 'X';
        }
        buffer.insert(buffer.end(), glyph_row, glyph_row + bytes);
    }
#else
    for (int y = 0; y < bitmap.rows; y++) {
        auto row = bitmap.buffer + bitmap.pitch * y;
        for (int k = 0; k < bytes; k++) {
            buffer.push_back(row[k]);
        }
    }
#endif
}

void font_generator::generate_bitmap(std::fstream &fs) {
    std::string progmem = use_progmem ? " PROGMEM" : "";
    fs << "const uint8_t " << font_name << "_bitmap[]" << progmem << " = {";

    auto size = buffer.size();
    auto ptr = buffer.data();
    fs << std::hex;
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            fs << std::endl;
        }

        fs << "0x" << std::setfill('0') << std::setw(2) << std::right << static_cast<int>(*ptr++) << ", ";
    }
    fs << "0x00 };" << std::endl
       << std::endl;
}

void font_generator::generate_glyphs(std::fstream &fs) {
    auto size = glyphs.size();
    auto g = glyphs.data();
    std::string progmem = use_progmem ? " PROGMEM" : "";
    fs << "static const zoal::text::glyph " << font_name << "_glyphs[]" << progmem << " = {" << std::endl;
    for (size_t i = 0; i < size; i++, g++) {
        fs << "{ 0x" << std::hex << (int) g->bitmap_offset;
        fs << ", " << std::dec << (int) g->width;
        fs << ", " << std::dec << (int) g->height;
        fs << ", " << std::dec << (int) g->x_advance;
        fs << ", " << std::dec << (int) g->x_offset;
        fs << ", " << std::dec << (int) g->y_offset << "}";

        if (i + 1 < size) {
            fs << "," << std::endl;
        } else {
            fs << std::endl;
        }
    }

    fs << "};" << std::endl
       << std::endl;
}

void font_generator::generate_ranges(std::fstream &fs) {
    fs << "static const zoal::text::unicode_range " << font_name << "_ranges[] = {" << std::endl;

    auto size = ranges.size();
    auto rng = ranges.data();
    for (size_t i = 0; i < size; i++, rng++) {
        fs << std::hex << "{ 0x" << rng->start << ", 0x" << rng->end << ", 0x" << rng->base << "}";
        if (i + 1 < size) {
            fs << "," << std::endl;
        } else {
            fs << std::endl;
        }
    }
    fs << "};" << std::endl
       << std::endl;
}

void font_generator::read_kering(FT_Face face) {
    FT_ULong from = 0xFFFF;
    FT_ULong to = 0x0000;

    for (auto &r : ranges) {
        if (from > r.start) {
            from = r.start;
        }

        if (to < r.end) {
            to = r.end;
        }
    }

    std::map<std::string, std::vector<FT_UInt>> kern_map;
    size_t count = 0;
    for (auto i = from; i <= to; i++) {
        if (!in_range(i)) {
            continue;
        }

        for (auto j = from; j <= to; j++) {
            if (!in_range(j)) {
                continue;
            }

            FT_UInt a = FT_Get_Char_Index(face, i);
            FT_UInt b = FT_Get_Char_Index(face, j);
            FT_Vector v;
            FT_Error error = FT_Get_Kerning(face, a, b, FT_KERNING_DEFAULT, &v);
            if (error) {
                return;
            }

            int x = v.x / 64;
            if (x != 0) {
                zoal::text::kerning_pair kd{(uint16_t) i, (uint16_t) j, (int8_t) x};
                kerning.push_back(kd);
            }
        }
    }
}

void font_generator::gen_kerning(std::fstream &fs, FT_Face face) {
    std::string progmem = use_progmem ? " PROGMEM" : "";
    fs << "static const zoal::text::kerning_pair " << font_name << "_kerning[] " << progmem << " = {" << std::endl;

    if (!FT_HAS_KERNING(face)) {
        fs << "};" << std::endl
           << std::endl;
        return;
    }

    for (auto iter = kerning.begin(); iter != kerning.end(); ++iter) {
        if (iter != kerning.begin()) {
            fs << ", " << std::endl;
        }

        fs << "{ 0x" << std::hex << (int) iter->first;
        fs << ", 0x" << std::hex << (int) iter->second;
        fs << ", " << std::dec << (int) iter->x_advance;
        fs << "}";
    }

    fs << std::endl
       << "};" << std::endl;
}

void font_generator::generate_font(std::fstream &fs) const {
    fs << "const zoal::text::font " << font_name << "{";
    fs << std::dec << (int) font_size << ", ";
    fs << font_name << "_bitmap, ";
    fs << font_name << "_glyphs, ";
    fs << std::dec << glyphs.size() << ", ";
    fs << font_name << "_ranges,";
    fs << std::dec << ranges.size();
    if (use_kern) {
        fs << ", " << font_name << "_kerning, " << std::dec << kerning.size();
    } else {
        fs << ", nullptr, 0";
    }

    fs << "};" << std::endl;
}
