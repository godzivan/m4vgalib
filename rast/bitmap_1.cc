#include "vga/rast/bitmap_1.h"

#include <cstdint>

#include "etl/assert.h"
#include "etl/prediction.h"

#include "vga/arena.h"
#include "vga/copy_words.h"
#include "vga/vga.h"
#include "vga/rast/unpack_1bpp.h"

using std::uint32_t;

namespace vga {
namespace rast {

Bitmap_1::Bitmap_1(unsigned width, unsigned height, unsigned top_line)
  : Bitmap_1(width, height, nullptr, top_line) {}

Bitmap_1::Bitmap_1(unsigned width,
                   unsigned height,
                   Pixel const * background,
                   unsigned top_line)
  : _lines(height),
    _words_per_line(width / 32),
    _top_line(top_line),
    _page1(false),
    _flip_pended(false),
    _clut{ 0, 0xFF },
    _fb{ arena_new_array<uint32_t>(_words_per_line * _lines),
         arena_new_array<uint32_t>(_words_per_line * _lines) },
    _background{background}
{}

Bitmap_1::~Bitmap_1() {
  _fb[0] = _fb[1] = nullptr;
  _background = nullptr;
}

__attribute__((section(".ramcode")))
Rasterizer::RasterInfo Bitmap_1::rasterize(unsigned cycles_per_pixel,
                                           unsigned line_number,
                                           Pixel *target) {
  line_number -= _top_line;
  if (ETL_UNLIKELY(line_number == 0)) {
    if (_flip_pended.exchange(false)) flip_now();
  } else if (ETL_UNLIKELY(line_number >= _lines)) {
    return { 0, 0, cycles_per_pixel, 0 };
  }

  uint32_t const *src = _fb[_page1] + _words_per_line * line_number;

  if (_background) {
    auto bg = _background + (_words_per_line * 32) * line_number;
    unpack_1bpp_overlay_impl(src, _clut, target, _words_per_line, bg);
  } else {
    unpack_1bpp_impl(src, _clut, target, _words_per_line);
  }

  return {
    .offset = 0,
    .length = _words_per_line * 32,
    .cycles_per_pixel = cycles_per_pixel,
    .repeat_lines = 0,
  };
}

Bitmap Bitmap_1::get_bg_bitmap() const {
  return { _fb[!_page1],
           _words_per_line * 32,
           _lines,
           static_cast<int>(_words_per_line) };
}

Graphics1 Bitmap_1::make_bg_graphics() const {
  ETL_ASSERT(can_bg_use_bitband());

  return Graphics1(get_bg_bitmap());
}

void Bitmap_1::pend_flip() {
  _flip_pended = true;
}

void Bitmap_1::flip_now() {
  _page1 = !_page1;
}

bool Bitmap_1::can_fg_use_bitband() const {
  unsigned addr = reinterpret_cast<unsigned>(_fb[_page1]);
  return (addr >= 0x20000000 && addr < 0x20100000)
      || (addr < 0x100000);
}

bool Bitmap_1::can_bg_use_bitband() const {
  unsigned addr = reinterpret_cast<unsigned>(_fb[!_page1]);
  return (addr >= 0x20000000 && addr < 0x20100000)
      || (addr < 0x100000);
}

void Bitmap_1::copy_bg_to_fg() const {
  copy_words(_fb[!_page1],
             _fb[_page1],
             _words_per_line * _lines);
}

void Bitmap_1::set_fg_color(Pixel c) {
  _clut[1] = c;
}

void Bitmap_1::set_bg_color(Pixel c) {
  _clut[0] = c;
}

}  // namespace rast
}  // namespace vga
