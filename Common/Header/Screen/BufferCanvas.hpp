#ifndef XCSOAR_SCREEN_BUFFER_CANVAS_HPP
#define XCSOAR_SCREEN_BUFFER_CANVAS_HPP

#include "Screen/VirtualCanvas.hpp"

class BufferCanvas : public VirtualCanvas {
protected:
  HBITMAP bitmap;

public:
  BufferCanvas():bitmap(NULL) {}
  BufferCanvas(const Canvas &canvas, unsigned _width, unsigned _height);
  ~BufferCanvas();

  void set(const Canvas &canvas, unsigned _width, unsigned _height);
  void set(const Canvas &canvas);
  void reset();

  void resize(unsigned _width, unsigned _height);
};

#endif
