/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/Font.hpp"
#include "Screen/Debug.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/AnyCanvas.hpp"
#include "Look/FontDescription.hpp"
#include "Asset.hpp"

#include <assert.h>

bool
Font::Load(const FontDescription &d)
{
  assert(IsScreenInitialized());

  Destroy();

  font = ::CreateFontIndirect(&(const LOGFONT &)d);
  if (font == nullptr)
    return false;

  if (GetObjectType(font) != OBJ_FONT) {
    Destroy();
    return false;
  }

  CalculateHeights();

  return true;
}

PixelSize
Font::TextSize(const TCHAR *text) const
{
  AnyCanvas canvas;
  canvas.Select(*this);
  return canvas.CalcTextSize(text);
}

void
Font::CalculateHeights()
{
  AnyCanvas canvas;
  canvas.Select(*this);

  TEXTMETRIC tm;
  ::GetTextMetrics(canvas, &tm);

  height = tm.tmHeight;
  ascent_height = tm.tmAscent;
  capital_height = tm.tmAscent - 1 - tm.tmHeight / 10;
}

void
Font::Destroy()
{
  if (font != nullptr) {
    assert(IsScreenInitialized());

#ifndef NDEBUG
    bool success =
#endif
      ::DeleteObject(font);
    assert(success);

    font = nullptr;
  }
}
