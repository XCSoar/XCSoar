#ifndef XCSOAR_SCREEN_VIRTUAL_CANVAS_HPP
#define XCSOAR_SCREEN_VIRTUAL_CANVAS_HPP

#include "Screen/Canvas.hpp"

class VirtualCanvas : public Canvas {
public:
  VirtualCanvas() {}
  VirtualCanvas(const Canvas &canvas, unsigned _width, unsigned _height);
  ~VirtualCanvas();

  void set(unsigned _width, unsigned _height);
  void set(const Canvas &canvas, unsigned _width, unsigned _height);
  void set(const Canvas &canvas);
  void reset();
};

#endif
