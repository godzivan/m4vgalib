#ifndef VGA_RAST_BITMAP_1_H
#define VGA_RAST_BITMAP_1_H

#include "vga/rasterizer.h"
#include "vga/graphics_1.h"

namespace vga {
namespace rast {

class Bitmap_1 : public Rasterizer {
public:
  Bitmap_1(unsigned width, unsigned height);

  virtual void activate(Timing const &);
  virtual LineShape rasterize(unsigned, Pixel *);
  virtual void deactivate();

  Graphics1 make_bg_graphics() const;
  void flip();

  void set_fg_color(Pixel);
  void set_bg_color(Pixel);

private:
  unsigned char *_fb[2];
  unsigned _lines;
  unsigned _bytes_per_line;
  bool _page1;
  Pixel _clut[2];
};

}  // namespace rast
}  // namespace vga

#endif  // VGA_RAST_BITMAP_1_H