#include "Screen/VirtualCanvas.hpp"

VirtualCanvas::VirtualCanvas(const Canvas &canvas,
                             unsigned _width, unsigned _height)
  :Canvas(::CreateCompatibleDC(canvas), _width, _height)
{
}

VirtualCanvas::~VirtualCanvas()
{
  reset();
}

void
VirtualCanvas::set(unsigned _width, unsigned _height)
{
  reset();
  Canvas::set(CreateCompatibleDC(NULL), _width, _height);
}

void
VirtualCanvas::set(const Canvas &canvas, unsigned _width, unsigned _height)
{
  reset();
  Canvas::set(CreateCompatibleDC(canvas), _width, _height);
}

void
VirtualCanvas::set(const Canvas &canvas)
{
  set(canvas, canvas.get_width(), canvas.get_height());
}

void VirtualCanvas::reset()
{
  if (dc != NULL)
    ::DeleteDC(dc);
}
