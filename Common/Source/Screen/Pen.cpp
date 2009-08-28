#include "Screen/Pen.hpp"

void
Pen::set(enum style style, unsigned width, const Color c)
{
  reset();
  pen = ::CreatePen(style, width, c);
}

void
Pen::set(unsigned width, const Color c)
{
  set(SOLID, width, c);
}

void
Pen::reset()
{
  if (pen != NULL) {
    ::DeleteObject(pen);
    pen = NULL;
  }
}
