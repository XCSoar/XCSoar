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
#include "ui/canvas/gdi/GdiPlusBitmap.hpp"

#include "Screen/Debug.hpp"
#include "system/Path.hpp"

#include <cassert>

Bitmap::Bitmap(Bitmap &&src)
  :bitmap(src.bitmap)
{
  src.bitmap = nullptr;
}

bool
Bitmap::LoadFile(Path path)
{
  bitmap = GdiLoadImage(path.c_str());
  return IsDefined();
}

void
Bitmap::Reset()
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
Bitmap::GetSize() const
{
  assert(IsDefined());

  BITMAP bm;
  ::GetObject(bitmap, sizeof(bm), &bm);
  const PixelSize size = { bm.bmWidth, bm.bmHeight };
  return size;
}
