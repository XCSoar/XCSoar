/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

  if (IsAltair()) {
    // JMW: don't know why we need this in GNAV, but we do.

    BufferCanvas buffer(canvas, {tm.tmAveCharWidth, tm.tmHeight});
    const HWColor white = buffer.map(COLOR_WHITE);

    buffer.SetBackgroundOpaque();
    buffer.SetBackgroundColor(COLOR_WHITE);
    buffer.SetTextColor(COLOR_BLACK);
    buffer.Select(*this);

    PixelRect rec;
    rec.left = 0;
    rec.top = 0;
    rec.right = tm.tmAveCharWidth;
    rec.bottom = tm.tmHeight;
    buffer.DrawOpaqueText(0, 0, rec, _T("M"));

    unsigned top = tm.tmHeight, bottom = 0;

    for (unsigned x = 0; x < (unsigned)tm.tmAveCharWidth; ++x) {
      for (unsigned y = 0; y < (unsigned)tm.tmHeight; ++y) {
        if (buffer.GetPixel(x, y) != white) {
          if (top > y)
            top = y;
          if (bottom < y)
            bottom = y;
        }
      }
    }

    capital_height = bottom - top + 1;
  } else {
    // This works for PPC
    capital_height = tm.tmAscent - 1 - tm.tmHeight / 10;
  }
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
