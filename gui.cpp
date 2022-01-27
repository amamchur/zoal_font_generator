#include "mainwindow.h"

#include <QApplication>

#include "font_generator.h"
#include <boost/program_options.hpp>
#include <iostream>

#include <zoal/gfx/glyph_renderer.hpp>
#include <zoal/gfx/renderer.hpp>
#include <zoal/ic/sh1106.hpp>
#include <zoal/io/output_stream.hpp>

font_generator generator;
extern uint8_t canvas[1024];

using adapter = zoal::ic::sh1106_adapter_0<128, 64>;
using graphics = zoal::gfx::renderer<uint8_t, adapter>;

class mem_reader {
public:
    template<class T>
    static inline const T& read_mem(const void *ptr) {
        return *reinterpret_cast<const T *>(ptr);
    }
};

int main(int argc, char *argv[]) {
    namespace po = boost::program_options;
    po::options_description desc("Options");

    desc.add_options()("help,h", "display help")("font,f", po::value<std::string>(), "path to font")("size,s", po::value<int>(), "font size")("progmem", "use PROGMEM")("kern", "use kerning")("ranges,r", po::value<std::vector<std::string>>(), "unicode char ranges: 0x0020-0x007")("name,n", po::value<std::string>(), "output font name");

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

    generator.font_name = vm["name"].as<std::string>();
    generator.font_ranges = vm["ranges"].as<std::vector<std::string>>();
    generator.font_path = vm["font"].as<std::string>();
    generator.font_size = vm["size"].as<int>();

    if (vm.count("progmem")) {
        generator.use_progmem = true;
    }
    if (vm.count("kern")) {
        generator.use_kern = true;
    }
//
//    FT_Library library;
//    FT_Error error = FT_Init_FreeType(&library);
//    if (error) {
//        return -1;
//    }

    generator.generate_fonts_file();

    auto g = graphics::from_memory(canvas);
    zoal::gfx::glyph_renderer<graphics, mem_reader> gl(g, &generator.current_font);
    zoal::io::output_stream<zoal::gfx::glyph_renderer<graphics, mem_reader>> text_stream(gl);
    g->clear(0);
    gl.color(1);
    gl.position(10, generator.current_font.y_advance);
    text_stream << "Hello World";

//    gl.position(10, generator.current_font.y_advance * 2);
//    gl.draw(L"Еней був парубок моторний");
//
//    gl.position(10, generator.current_font.y_advance * 3);
//    gl.draw(L"І хлопець хоть куди козак,");
//
//    gl.position(10, generator.current_font.y_advance * 4);
//    gl.draw(L"Удавсь на всеє зле проворний,");

    gl.position(10, generator.current_font.y_advance * 2);
    gl.draw(L"ІіЇї₴ЄєґҐ");

    QApplication app(argc, argv);
    MainWindow wnd;
    wnd.show();
    return app.exec();
}

const char path[] = "D:/Roboto-Regular.ttf";
int main_() {
    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        return -1;
    }

    FT_Face face;
    error = FT_New_Face(library, path, 0, &face);
    if (error) {
        FT_Done_FreeType(library);
        return -1;
    }

    error = FT_Set_Pixel_Sizes(face, 0, 16);
    if (error) {
        return -1;
    }

    FT_UInt glyph_index = FT_Get_Char_Index(face, 'H');
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
        return -1;
    }
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        return -1;
    }

    FT_Bitmap *bitmap = &face->glyph->bitmap;
    printf("pitch: %d\r\n", bitmap->pitch);
    printf("width: %d\r\n", bitmap->width);
    printf("rows: %d\r\n", bitmap->rows);

    char str[256];
    for (int y = 0; y < bitmap->rows; y++) {
        uint8_t *row = bitmap->buffer + bitmap->pitch * y;
        memset(str, 0, sizeof(str));
        int index = 0;
        for (int k = 0; k < bitmap->width; k++) {
            uint8_t b = row[k];
            str[index++] = b < 50 ? ' ' : 'X';
        }
        printf("%s\r\n", str);
    }
    printf("\r\n");
    FT_Done_FreeType(library);

    return 0;
}
