#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstring>
#include <iomanip>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include <map>
#include "types.hpp"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::program_options;

struct kern_data {
    uint16_t first;
    uint16_t second;
    int8_t x_advance;
};

std::string font_name = "roboto_regular_24";
std::vector<uint8_t> buffer;
std::vector<zoal::text::unicode_range> ranges;
std::vector<zoal::text::glyph> glyphs;
std::string font_path;
std::vector<std::string> font_ranges;
std::vector<kern_data> kerning;

uint8_t font_size = 16;
bool use_progmem = false;
bool use_kern = false;

bool in_range(FT_ULong i);

void create_bitmap_glyph(FT_GlyphSlot slot) {
    FT_Bitmap *bitmap = &slot->bitmap;
    int pitch = abs(bitmap->pitch);
    zoal::text::glyph g{};
    g.bitmap_offset = buffer.size();
    g.x_offset = (int8_t) slot->bitmap_left;
    g.y_offset = (int8_t) (-slot->bitmap_top);
    g.width = bitmap->width;
    g.height = bitmap->rows;
    g.x_advance = slot->advance.x >> 6;
    glyphs.push_back(g);

    for (int y = 0; y < bitmap->rows; y++) {
        auto row = bitmap->buffer + pitch * y;
        auto bytes = ((bitmap->width + 7) >> 3);
        for (int k = 0; k < bytes; k++) {
            buffer.push_back(row[k]);
        }
#if 0
        for (int x = 0; x < bitmap->width; x++) {
            auto value = row[x >> 3];
            auto b = (value & (128 >> (x & 7)))  ? 1 : 0;
            if (b)
                std::cout << "@";
            else
                std::cout << ".";
        }

        for (int k = 0; k < bytes; k++) {
            using namespace std;
            cout << ", 0x" << setfill('0') << setw(2) << right << hex << (int)row[k];
        }

        std::cout << std::endl;
#endif
    }
}

void make_range(FT_Face face, FT_ULong range_from, FT_ULong range_to) {
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

//        std::cout << "Unicode: 0x" << std::setfill('0') << std::setw(4) << std::right << std::hex << code << std::endl;
        create_bitmap_glyph(slot);
    }
}

void generate_bitmap(std::fstream &fs) {
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
    fs << "0x00 };" << std::endl << std::endl;
}

void generate_ranges(std::fstream &fs) {
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
    fs << "};" << std::endl << std::endl;
}

void generate_glyphs(std::fstream &fs) {
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

    fs << "};" << std::endl << std::endl;
}

void generate_font(std::fstream &fs) {
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

void gen_kerning(std::fstream &fs, FT_Face face) {
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

    std::string progmem = use_progmem ? " PROGMEM" : "";
        fs << "static const zoal::text::kerning_pair " << font_name << "_kerning[] " << progmem <<" = {" << std::endl;

    if (!FT_HAS_KERNING(face)) {
        fs << "};" << std::endl << std::endl;
        return;
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
                kern_data kd{(uint16_t) i, (uint16_t) j, (int8_t) x};
                kerning.push_back(kd);
            }
        }
    }

    for (auto iter = kerning.begin(); iter != kerning.end(); ++iter) {
        if (iter != kerning.begin()) {
            fs << ", " << std::endl;
        }

        fs << "{ 0x" << std::hex << (int)iter->first;
        fs << ", 0x" << std::hex << (int)iter->second;
        fs << ", " << std::dec << (int)iter->x_advance;
        fs << "}";
    }

    fs << std::endl << "};" << std::endl;
}

void generate_src(FT_Face face) {
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

bool in_range(FT_ULong value) {
    for (auto &r : ranges) {
        if (r.start <= value && value <= r.end) {
            return true;
        }
    }
    return false;
}


int generate_fonts_file() {
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

    std::cout << "Buffer size: " << std::dec << buffer.size() << std::endl;
    std::cout << "Total glyphs: " << std::dec << glyphs.size() << std::endl;

    generate_src(face);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

int main(int argc, char *argv[]) {
    try {
        namespace po = boost::program_options;
        po::options_description desc("Options");

        desc.add_options()
                ("help,h", "display help")
                ("font,f", po::value<std::string>(), "path to font")
                ("size,s", po::value<int>(), "font size")
                ("progmem", "use PROGMEM")
                ("kern", "use kerning")
                ("ranges,r", po::value<std::vector<std::string>>(), "unicode char ranges: 0x0020-0x007")
                ("name,n", po::value<std::string>(), "output font name");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        if (!vm.count("font")) {
            std::cout << "Missing font argument parameter" << std::endl;
            return 0;
        }

        if (!vm.count("ranges")) {
            std::cout << "Missing ranges argument parameter" << std::endl;
            return 0;
        }

        if (!vm.count("size")) {
            std::cout << "Missing size argument parameter" << std::endl;
            return 0;
        }

        font_name = vm["name"].as<std::string>();
        font_ranges = vm["ranges"].as<std::vector<std::string>>();
        font_path = vm["font"].as<std::string>();
        font_size = vm["size"].as<int>();

        if (vm.count("progmem")) {
            use_progmem = true;
        }
        if (vm.count("kern")) {
            use_kern = true;
        }

        generate_fonts_file();
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
