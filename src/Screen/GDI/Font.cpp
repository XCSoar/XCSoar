/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Asset.hpp"

#include <assert.h>

bool
Font::Load(const TCHAR* facename, UPixelScalar height, bool bold, bool italic)
{
  LOGFONT font;
  memset((char *)&font, 0, sizeof(LOGFONT));

  _tcscpy(font.lfFaceName, facename);
  font.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  font.lfHeight = (long)height;
  font.lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font.lfItalic = italic;
  if (IsAltair()) // better would be: if (screen.dpi() < 100)
    font.lfQuality = NONANTIALIASED_QUALITY;
  else
    font.lfQuality = ANTIALIASED_QUALITY;
  return Font::Load(font);
}

bool
Font::Load(const LOGFONT &log_font)
{
  assert(IsScreenInitialized());

  Destroy();

  font = ::CreateFontIndirect(&log_font);
  if (font == NULL)
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

    BufferCanvas buffer(canvas, tm.tmAveCharWidth, tm.tmHeight);
    const HWColor white = buffer.map(COLOR_WHITE);

    buffer.SetBackgroundOpaque();
    buffer.SetBackgroundColor(COLOR_WHITE);
    buffer.SetTextColor(COLOR_BLACK);
    buffer.Select(*this);

    RECT rec;
    rec.left = 0;
    rec.top = 0;
    rec.right = tm.tmAveCharWidth;
    rec.bottom = tm.tmHeight;
    buffer.DrawOpaqueText(0, 0, rec, _T("M"));

    UPixelScalar top = tm.tmHeight, bottom = 0;

    for (UPixelScalar x = 0; x < (UPixelScalar)tm.tmAveCharWidth; ++x) {
      for (UPixelScalar y = 0; y < (UPixelScalar)tm.tmHeight; ++y) {
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
  if (font != NULL) {
    assert(IsScreenInitialized());

    ::DeleteObject(font);
    font = NULL;
  }
}
