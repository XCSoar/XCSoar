#include "Screen/Brush.hpp"
#include "Screen/Bitmap.hpp"

void
Brush::set(const Color c)
{
  reset();
  brush = ::CreateSolidBrush(c);
}

void
Brush::set(const Bitmap &bitmap)
{
  reset();
  brush = ::CreatePatternBrush(bitmap.native());
}

void
Brush::reset()
{
  if (brush != NULL) {
    ::DeleteObject(brush);
    brush = NULL;
  }
}
