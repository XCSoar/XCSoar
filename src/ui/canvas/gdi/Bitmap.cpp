// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/gdi/GdiPlusBitmap.hpp"

#include "Screen/Debug.hpp"
#include "system/Path.hpp"

#include <cassert>

#include <wingdi.h>
#include <winuser.h>

Bitmap::Bitmap(Bitmap &&src) noexcept
  :bitmap(std::exchange(src.bitmap, nullptr))
{
}

Bitmap &Bitmap::operator=(Bitmap &&src) noexcept
{
  using std::swap;
  swap(bitmap, src.bitmap);
  return *this;
}

bool
Bitmap::LoadFile(Path path)
{
  bitmap = GdiLoadImage(path.c_str());
  return IsDefined();
}

void
Bitmap::Reset() noexcept
{
  if (bitmap != nullptr) {
    assert(IsScreenInitialized());

#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(bitmap);
    assert(success);

    bitmap = nullptr;
  }
}

PixelSize
Bitmap::GetSize() const noexcept
{
  assert(IsDefined());

  BITMAP bm;
  ::GetObject(bitmap, sizeof(bm), &bm);
  const PixelSize size = { bm.bmWidth, bm.bmHeight };
  return size;
}
