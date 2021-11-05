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
#include "font_generator.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::program_options;

int main(int argc, char *argv[]) {
    font_generator gen;

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

        gen.font_name = vm["name"].as<std::string>();
        gen.font_ranges = vm["ranges"].as<std::vector<std::string>>();
        gen.font_path = vm["font"].as<std::string>();
        gen.font_size = vm["size"].as<int>();

        if (vm.count("progmem")) {
            gen.use_progmem = true;
        }
        if (vm.count("kern")) {
            gen.use_kern = true;
        }

        gen.generate_fonts_file();
    } catch (std::exception &exc) {
        std::cerr << exc.what() << std::endl;
    }

    return 0;
}
