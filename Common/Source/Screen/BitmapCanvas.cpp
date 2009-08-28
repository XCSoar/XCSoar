#include "Screen/BitmapCanvas.hpp"
#include "Screen/Bitmap.hpp"

void
BitmapCanvas::select(const Bitmap &bitmap, unsigned _width, unsigned _height)
{
  old = (HBITMAP)SelectObject(dc, bitmap.native());
}

void BitmapCanvas::clear()
{
  SelectObject(dc, old);
  old = NULL;
}
