#ifndef XCSOAR_SCREEN_BITMAP_CANVAS_HPP
#define XCSOAR_SCREEN_BITMAP_CANVAS_HPP

#include "Screen/VirtualCanvas.hpp"

class Bitmap;

class BitmapCanvas : public VirtualCanvas {
protected:
  HBITMAP old;

public:
  BitmapCanvas():old(NULL) {}
  BitmapCanvas(const Canvas &canvas)
    :VirtualCanvas(canvas, 1, 1), old(NULL) {}

  void set()
  {
    VirtualCanvas::set(1, 1);
  }

  void set(const Canvas &canvas)
  {
    VirtualCanvas::set(canvas, 1, 1);
  }

  void select(const Bitmap &bitmap, unsigned _width=1, unsigned _height=1);

  void clear();
};

#endif
