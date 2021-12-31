/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
Bitmap::Load(ResourceId id, Type type)
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
