#include "Screen/BufferCanvas.hpp"

BufferCanvas::BufferCanvas(const Canvas &canvas, unsigned _width, unsigned _height)
  :VirtualCanvas(canvas, _width, _height)
{
  bitmap = ::CreateCompatibleBitmap(canvas, width, height);
  ::SelectObject(dc, bitmap);
}

BufferCanvas::~BufferCanvas()
{
  reset();
}

void BufferCanvas::set(const Canvas &canvas, unsigned _width, unsigned _height)
{
  reset();
  VirtualCanvas::set(canvas, _width, _height);
  bitmap = ::CreateCompatibleBitmap(canvas, width, height);
  ::SelectObject(dc, bitmap);
}

void
BufferCanvas::set(const Canvas &canvas)
{
  set(canvas, canvas.get_width(), canvas.get_height());
}

void BufferCanvas::reset()
{
  VirtualCanvas::reset();
  if (bitmap != NULL)
    ::DeleteObject(bitmap);
}

void BufferCanvas::resize(unsigned _width, unsigned _height)
{
  ::DeleteObject(bitmap);
  Canvas::resize(_width, _height);
  bitmap = ::CreateCompatibleBitmap(dc, width, height);
  ::SelectObject(dc, bitmap);
}
