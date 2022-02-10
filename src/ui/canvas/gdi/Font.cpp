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

#include "ui/canvas/Font.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "Screen/Debug.hpp"
#include "Look/FontDescription.hpp"
#include "util/TStringView.hxx"
#include "Asset.hpp"

#include <cassert>
#include <stdexcept>

void
Font::Load(const FontDescription &d)
{
  assert(IsScreenInitialized());

  Destroy();

  font = ::CreateFontIndirect(&(const LOGFONT &)d);
  if (font == nullptr)
    throw std::runtime_error{"CreateFontIndirect() failed"};

  if (GetObjectType(font) != OBJ_FONT) {
    Destroy();
    throw std::runtime_error{"CreateFontIndirect() did not return a font"};
  }

  CalculateHeights();
}

PixelSize
Font::TextSize(TStringView text) const noexcept
{
  AnyCanvas canvas;
  canvas.Select(*this);
  return canvas.CalcTextSize(text);
}

void
Font::CalculateHeights() noexcept
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
Font::Destroy() noexcept
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
