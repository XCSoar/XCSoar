#include "Screen/Bitmap.hpp"
#include "Interface.hpp"

Bitmap::~Bitmap()
{
  reset();
}

void
Bitmap::load(const TCHAR *name)
{
  reset();
  bitmap = LoadBitmap(hInst, name);
}

void
Bitmap::create(const BITMAPINFO *pbmi, VOID **ppvBits)
{
  HDC hDC = ::GetDC(NULL);
  bitmap = CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
  ::ReleaseDC(NULL, hDC);
}

void
Bitmap::reset()
{
  if (bitmap != NULL) {
    DeleteObject(bitmap);
    bitmap = NULL;
  }
}

const SIZE
Bitmap::get_size() const
{
  BITMAP bm;
  ::GetObject(bitmap, sizeof(bm), &bm);
  const SIZE size = { bm.bmWidth, bm.bmHeight };
  return size;
}
