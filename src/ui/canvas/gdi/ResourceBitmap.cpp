// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "ResourceLoader.hpp"
#include "ResourceId.hpp"

#include <cassert>

#include <wingdi.h>
#include <winuser.h>

Bitmap::Bitmap(ResourceId id)
{
  Load(id);
}

bool
Bitmap::Load(ResourceId id, [[maybe_unused]] Type type)
{
  Reset();

  bitmap = ResourceLoader::LoadBitmap2(id);
  return bitmap != nullptr;
}

bool
Bitmap::LoadStretch(ResourceId id, unsigned zoom)
{
  assert(zoom > 0);

  if (!Load(id))
    return false;

  if (zoom <= 1)
    return true;

  const PixelSize src_size = GetSize();
  PixelSize dest_size;
  dest_size.width = src_size.width * zoom;
  dest_size.height = src_size.height * zoom;

  HDC dc = ::GetDC(nullptr), src_dc = ::CreateCompatibleDC(dc),
    dest_dc = ::CreateCompatibleDC(dc);
  HBITMAP dest_bitmap = ::CreateCompatibleBitmap(dc,
                                                 dest_size.width, dest_size.height);
  ::ReleaseDC(nullptr, dc);

  if (dest_bitmap == nullptr) {
    ::DeleteDC(src_dc);
    ::DeleteDC(dest_dc);
    return false;
  }

  ::SelectObject(src_dc, bitmap);
  ::SelectObject(dest_dc, dest_bitmap);

  ::StretchBlt(dest_dc, 0, 0, dest_size.width, dest_size.height,
               src_dc, 0, 0, src_size.width, src_size.height,
               SRCCOPY);

  ::DeleteDC(src_dc);
  ::DeleteDC(dest_dc);

  ::DeleteObject(bitmap);
  bitmap = dest_bitmap;

  return true;
}
