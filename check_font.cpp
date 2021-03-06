//
// Created by amam on 6/17/2021.
//
#define PROGMEM

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cstring>
#include "roboto_regular_16.hpp"
#include "types.hpp"

constexpr int map_width = 24;
constexpr int map_height = 100;
char map[map_width][map_height] = {' '};

namespace zoal {
    namespace gfx {
        template<class Graphics>
        class glyph_render {
        public:
            using self_type = glyph_render<Graphics>;
            using pixel_type = typename Graphics::pixel_type;

            glyph_render(Graphics *g, const zoal::text::font *font) : graphics_(g), font_(font) {
            }

            void draw(const wchar_t ch, pixel_type fg) {
                const zoal::text::unicode_range *range = nullptr;
                auto code = (uint16_t) ch;
                for (int i = 0; i < font_->ranges_count; i++) {
                    const zoal::text::unicode_range *r = font_->ranges + i;
                    if (r->start <= code && code <= r->end) {
                        range = r;
                        break;
                    }
                }

                if (range == nullptr) {
                    return;
                }

                auto pos = code - range->start + range->base;
                auto glyph = font_->glyphs + pos;
                render_glyph(glyph);
            }

            void draw(const wchar_t *text, pixel_type fg) {
                while (*text) {
                    draw(*text++, fg);
                }
            }

            self_type &position(int x, int y) {
                x_ = x;
                y_ = y;
                return *this;
            }

        private:
            void render_glyph(const zoal::text::glyph *g) {
                const uint8_t *data = font_->bitmap + g->bitmap_offset;
                const uint8_t bytes_per_row = (g->width + 7) >> 3;
                for (int y = 0; y < g->height; y++) {
                    auto row = data + y * bytes_per_row;
                    for (int x = 0; x < g->width; x++) {
                        auto ptr = row + (x >> 3);
                        int mask = 0x80 >> (x & 7);
                        int value = *ptr & mask;
                        graphics_->pixel(y_ + y + g->y_offset, x_ + x + g->x_offset, (value ? 1 : 0));
                    }
                }
                x_ += g->x_advance;
            }

            const zoal::text::font *font_{nullptr};
            Graphics *graphics_;
            int x_{0};
            int y_{0};
        };
    }
}

class graphics {
public:
    using pixel_type = uint8_t;

    void pixel(int x, int y, pixel_type c) {
        if (x < 0 || x >= map_width || y < 0 || y >= map_height) {
            std::cout << "Oops..." << std::endl;
            return;
        }

        map[x][y] = c == 1 ? 'X' : '.';
    }
};


int8_t get_kerning(const zoal::text::font &font, uint16_t f, uint16_t s) {
    if (font.kerning_pairs_count == 0) {
        return 0;
    }

    auto l = font.kerning_pairs;
    auto r = l + font.kerning_pairs_count - 1;
    const zoal::text::kerning_pair *k = nullptr;
    while (l < r) {
        auto m = l + (r - l) / 2;
        zoal::text::kerning_pair kp;
        memcpy(&kp, m, sizeof(kp));
        if (m->first == f) {
            k = m;
            break;
        }

        if (m->first < f) {
            l = m;
        }

        if (m->first > f) {
            r = m;
        }
    }

    if (k == nullptr) {
        return 0;
    }

    l = font.kerning_pairs;
    r = l + font.kerning_pairs_count - 1;
    for (auto p = k; p >= l; p--) {
        zoal::text::kerning_pair kp;
        memcpy(&kp, p, sizeof(kp));
        if (p->second == s) {
            return p->x_advance;
        }
    }

    for (auto p = k; p <= r; p++) {
        zoal::text::kerning_pair kp;
        memcpy(&kp, p, sizeof(kp));
        if (p->second == s) {
            return p->x_advance;
        }
    }

    return 0;
}

int main() {
    int kerning = get_kerning(roboto_regular_16, 0x22u, 0x22u);
    std::cout << "kerning: " << kerning  << std::endl;

    std::cout << "Begin!" << std::endl;
    graphics g;
    zoal::gfx::glyph_render<graphics> gr(&g, &roboto_regular_16);

    memset(map, '_', sizeof(map));
    for (int i = 0; i < map_width; i++) {
        map[i][map_height - 1] = '\0';
    }

    gr.position(0, 23);
    gr.draw(L"H", 1);

    for (int i = 0; i < map_width; i++) {
        if (*map[i] == '\0') {
            break;
        }
        std::cout << map[i] << std::endl;
    }

    std::cout << "End!" << std::endl;

    return 0;
}
